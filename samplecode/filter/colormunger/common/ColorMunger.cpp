// ADOBE SYSTEMS INCORPORATED
// Copyright  1993 - 2002 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this 
// file in accordance with the terms of the Adobe license agreement
// accompanying it.  If you have received this file from a source
// other than Adobe, then your use, modification, or distribution
// of it requires the prior written permission of Adobe.
//-------------------------------------------------------------------
//-------------------------------------------------------------------------------
//
//	File:
//		ColorMunger.cpp
//
//	Description:
//		This file contains the functions and source
//		Filter module ColorMunger, a module exemplifying
//		the use of the Color Services suite.
//
//	Use:
//		This module takes a source color of any color space
//		and converts it to a target color in any color
//		space.  It shows how to convert colors as well as
//		pop the color picker.  It appears in
//		Filters>>Utilities>>ColorMunger.
//
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//	Includes 
//-------------------------------------------------------------------------------

#include "ColorMunger.h"

//��Ƭ��С��64 * 64
#define	TILESIZE			64*4 

//������󶶶����루30���أ�
#define MAX_DISTANCE		30
//-------------------------------------------------------------------------------
//	Prototypes.
//-------------------------------------------------------------------------------
//=======================================
//		ȫ�ֱ���
//=======================================
//dll instance
HINSTANCE		dllInstance;
FilterRecord*	gFilterRecord;
int32*			gData; 
//SPBasicSuite*	sSPBasic;
Rect			m_TileIn;		//��ȡ���Σ���TileOutҪ�󣡣�
Rect			m_TileOut;		//д�ؾ��Σ�64*64����Ƭ��
BOOL			m_ShowUI;		//�Ƿ���Ҫ��ʾ�Ի��� 

void InitGlobals (Ptr globalPtr);		  	// Initialize globals.
void DoStart (GPtr globals);				// Main routine.

void ValidateParameters (GPtr globals);		// Validates (inits) parameters.
void DoParameters (GPtr globals);
void DoPrepare (GPtr globals);
void DoContinue (GPtr globals);
void DoFinish (GPtr globals);
//������������������
void CopyPsRect(Rect* src, Rect* dest)
{
	dest->left = src->left;
	dest->top = src->top;
	dest->right = src->right;
	dest->bottom = src->bottom;
}

//������������һ��������Ϊ�վ���
void ZeroPsRect(Rect* dest)
{
	dest->left = 0;
	dest->top = 0;
	dest->right = 0;
	dest->bottom = 0;
}
//-------------------------------------------------------------------------------
//	Globals -- Define global variables for plug-in scope.
//-------------------------------------------------------------------------------

SPBasicSuite * sSPBasic = NULL;

//-------------------------------------------------------------------------------
//
//	PluginMain / main
//
//	All calls to the plug-in module come through this routine.
//	It must be placed first in the resource.  To achieve this,
//	most development systems require this be the first routine
//	in the source.
//
//	The entrypoint will be "pascal void" for Macintosh,
//	"void" for Windows.
//
//	Inputs:
//		const int16 selector						Host provides selector indicating
//													what command to do.
//
//		FilterRecord *filterParamBlock				Host provides pointer to parameter
//													block containing pertinent data
//													and callbacks from the host.
//													See PIFilter.h.
//
//	Outputs:
//		FilterRecord *filterParamBlock				Host provides pointer to parameter
//													block containing pertinent data
//													and callbacks from the host.
//													See PIFilter.h.
//
//		void *data									Use this to store a handle or pointer to our
//													global parameters structure, which
//													is maintained by the host between
//													calls to the plug-in.
//
//		int16 *result								Return error result or noErr.  Some
//													errors are handled by the host, some
//													are silent, and some you must handle.
//													See PIGeneral.h.
//
//-------------------------------------------------------------------------------

DLLExport MACPASCAL void PluginMain (const int16 selector,
						             FilterRecordPtr filterParamBlock,
						             intptr_t *data,
						             int16 *result)
{
  try {
	//---------------------------------------------------------------------------
	//	(1) Check for about box request.
	//
	// 	The about box is a special request; the parameter block is not filled
	// 	out, none of the callbacks or standard data is available.  Instead,
	// 	the parameter block points to an AboutRecord, which is used
	// 	on Windows.
	//---------------------------------------------------------------------------

	if (selector == filterSelectorAbout)
	{
		sSPBasic = ((AboutRecordPtr)filterParamBlock)->sSPBasic;
		DoAbout((AboutRecordPtr)filterParamBlock);
	}
	else
	{ // do the rest of the process as normal:

		sSPBasic = ((FilterRecordPtr)filterParamBlock)->sSPBasic;

		Ptr globalPtr = NULL;		// Pointer for global structure
		GPtr globals = NULL; 		// actual globals
		 
		gFilterRecord = (FilterRecordPtr)filterParamBlock;
		globalPtr = AllocateGlobals (result,
									 filterParamBlock,
									 filterParamBlock->handleProcs,
									 sizeof(Globals),
						 			 data,
						 			 InitGlobals);
		
		if (globalPtr == NULL)
		{ // Something bad happened if we couldn't allocate our pointer.
		  // Fortunately, everything's already been cleaned up,
		  // so all we have to do is report an error.
		  
		  *result = memFullErr;
		  return;
		}
		
		// Get our "globals" variable assigned as a Global Pointer struct with the
		// data we've returned:
		globals = (GPtr)globalPtr;

		//-----------------------------------------------------------------------
		//	(3) Dispatch selector.
		//-----------------------------------------------------------------------

		switch (selector)
		{
			case filterSelectorParameters:
			//	DoParameters(globals);
				break;
			case filterSelectorPrepare:
				DoPrepare(globals);
				break;
			case filterSelectorStart:
				DoStart(globals);
				break;
			case filterSelectorContinue:
				DoContinue(globals);
				break;
			case filterSelectorFinish:
				DoFinish(globals);
				break;
		}
	
		//-----------------------------------------------------------------------
		//	(4) Unlock data, and exit resource.
		//
		//	Result is automatically returned in *result, which is
		//	pointed to by gResult.
		//-----------------------------------------------------------------------	
		
		// unlock handle pointing to parameter block and data so it can move
		// if memory gets shuffled:
		if (*data != 0 /* NULL */ )
			PIUnlockHandle((Handle)*data);
	
	} // about selector special		
	
  } // end try
  catch(...)
  {
	  if (result != NULL)
		  *result = -1;
  }
} // end PluginMain

//-------------------------------------------------------------------------------
//
//	InitGlobals
//	
//	Initalize any global values here.  Called only once when global
//	space is reserved for the first time.
//
//	Inputs:
//		Ptr globalPtr		Standard pointer to a global structure.
//
//	Outputs:
//		Initializes any global values with their defaults.
//
//-------------------------------------------------------------------------------

void InitGlobals (Ptr globalPtr)
{	
	// create "globals" as a our struct global pointer so that any
	// macros work:
	GPtr globals = (GPtr)globalPtr;
	
	// Initialize global variables:
	ValidateParameters (globals);
	
} // end InitGlobals

//-------------------------------------------------------------------------------
//
//	ValidateParameters
//
//	Initialize parameters to default values.
//
//	Inputs:
//		GPtr globals		Pointer to global structure.
//
//	Outputs:
//		gSourceColor		Default: 0, 0, 0, 255.
//
//		gTargetColor		Default: 0, 0, 0, 255.
//
//		gColor				Default: 0.
//
//		gColorize			Default: 0.
//
//-------------------------------------------------------------------------------

void ValidateParameters (GPtr globals)
{
	if (gStuff->parameters == NULL)
	{ // We haven't created these yet.

		gStuff->parameters = PINewHandle ((long) sizeof (TParameters));

		if (gStuff->parameters != NULL)
		{ // Got it.  Fill out the fields.
			CSSetColor (gSourceColor, 0, 0, 0, 255);
			CSCopyColor (gTargetColor, gSourceColor);

			gColor = gColorize = 0;

		}
		else
		{ // Oops.  Couldn't allocate memory.
				
			gResult = memFullErr;
			return;
		}
	} // parameters
	
} // end ValidateParameters

//-------------------------------------------------------------------------------
//
//	DoParameters
//
//	Initialize parameters to default values.
//
//	Inputs:
//
//	Outputs:

/* Asks the user for the plug-in filter module's parameters. Note that
   the image size information is not yet defined at this point. Also, do
   not assume that the calling program will call this routine every time the
   filter is run (it may save the data held by the parameters handle in
   a macro file). Initialize any single-time info here. */

void DoParameters (GPtr globals)

{
	//m_ShowUI = TRUE;
	ValidateParameters (globals);
	
	gQueryForParameters = TRUE;
	/* If we're here, that means we're being called for the first time. */
}

/*****************************************************************************/

/* Prepare to filter an image.	If the plug-in filter needs a large amount
   of buffer memory, this routine should set the bufferSpace field to the
   number of bytes required. Also check for */

void DoPrepare (GPtr globals)
{	
	if (gFilterRecord != NULL)
	{
		//����׼���������ݣ�һ�����ڴ洢ԭʼ���ݣ�һ��������ʾ
		gFilterRecord->bufferSpace = 2 * 90 * 90 * gFilterRecord->planes;
		gFilterRecord->maxSpace = gFilterRecord->bufferSpace;
	}
	return;

	gStuff->bufferSpace = 0;
	CSSetColor (gSourceColor, 0, 255, 0, 0);
}

/*****************************************************************************/

/* Does all of the filtering. */

void DoStart (GPtr globals)
	{

	if (gFilterRecord == NULL)
		return;


	//���ǳ�ʼ����һ��TileOut
	m_TileOut.left = gFilterRecord->filterRect.left;
	m_TileOut.top = gFilterRecord->filterRect.top;
	m_TileOut.right = min(m_TileOut.left + TILESIZE, gFilterRecord->filterRect.right);
	m_TileOut.bottom = min(m_TileOut.top + TILESIZE, gFilterRecord->filterRect.bottom);

	//���õ�һ��TileIn����TileOut�Ļ�������������չdistance���룩
	m_TileIn.left = max(m_TileOut.left - 22, gFilterRecord->filterRect.left);
	m_TileIn.top = max(m_TileOut.top - 22, gFilterRecord->filterRect.top);
	m_TileIn.right = min(m_TileOut.right + 22, gFilterRecord->filterRect.right);
	m_TileIn.bottom = min(m_TileOut.bottom + 22, gFilterRecord->filterRect.bottom);

	//����inRect, outRect
	CopyPsRect(&m_TileIn, &gFilterRecord->inRect);//����������Ҫ�����outRectһ��������
	CopyPsRect(&m_TileOut, &gFilterRecord->outRect);

	//����ȫ��ͨ����������Ϊinterleave�ֲ���
	gFilterRecord->inLoPlane = 0;
	gFilterRecord->inHiPlane = (gFilterRecord->planes - 1);;
	gFilterRecord->outLoPlane = 0;
	gFilterRecord->outHiPlane = (gFilterRecord->planes - 1);
	
	return;
	if (!WarnAdvanceStateAvailable ())
	{
		gResult = userCanceledErr; // exit gracefully
		goto done;
	}

	if (!WarnColorServicesAvailable())
	{
		gResult = userCanceledErr; // exit gracefully
		goto done;
	}
	
	ValidateParameters (globals);
	/* if stuff hasn't been initialized that we need, do it,
		then go check if we've got scripting commands to
		override our settings */
	gQueryForParameters = ReadScriptParams (globals); // update our parameters with the scripting parameters, if available

	if (gQueryForParameters)
	{
		gQueryForParameters = FALSE;
		if (!DoUI (globals))
			goto done; // canceled
	}

	if (gResult != noErr)
		goto done;

	done:
	
	// if one screws with the outData with a proxy, reset that here
		
	PISetRect (&gStuff->inRect, 0, 0, 0, 0);
	PISetRect (&gStuff->outRect, 0, 0, 0, 0);
	PISetRect (&gStuff->maskRect, 0, 0, 0, 0);

	}

/*****************************************************************************/

/* Call routines to filter the area. */

void DoFilterRect (GPtr globals)
{

	// do actual filtering here

	PISetRect (&gStuff->inRect, 0, 0, 0, 0);
	PISetRect (&gStuff->outRect, 0, 0, 0, 0);
	PISetRect (&gStuff->maskRect, 0, 0, 0, 0);
	
}

/*****************************************************************************/

/* Given that we do all of the filtering during the start phase, the continue
   phase is negligible. */

void DoContinue (GPtr globals)
	{
	int indexIn, indexOut, x, y;	//��������

	if (gFilterRecord == NULL)
		return;
	

	//��λ����
	int planes = gFilterRecord->outHiPlane - gFilterRecord->outLoPlane + 1; //ͨ������

																			//�����ɫ
	uint8 r = 0;
	uint8 g = 0;
	uint8 b = 0;
	uint8 a = 0;
	//��͸����

	uint8 *pDataIn = (uint8*)gFilterRecord->inData;
	uint8 *pDataOut = (uint8*)gFilterRecord->outData;

	//ɨ���п�ȣ��ֽڣ�
	int strideIn = gFilterRecord->inRowBytes;
	int strideOut = gFilterRecord->outRowBytes;

	//���ǰ�������ο����� m_Tile
	CopyPsRect(&gFilterRecord->inRect, &m_TileIn);
	CopyPsRect(&gFilterRecord->outRect, &m_TileOut);

	//���ñ߽�
	int imaxOut = (m_TileOut.right - m_TileOut.left);
	int jmaxOut = (m_TileOut.bottom - m_TileOut.top);
	int imaxIn = (m_TileIn.right - m_TileIn.left);
	int jmaxIn = (m_TileIn.bottom - m_TileIn.top);

	//��ȡ�������Σ�inRect��outRect��֮���ƫ�ƣ��� outRect ���Ͻ��� inRect ���е�����
	int x0 = gFilterRecord->outRect.left - gFilterRecord->inRect.left;
	int y0 = gFilterRecord->outRect.top - gFilterRecord->inRect.top;

	for (int j = 0; j< jmaxOut; j++)
	{
		for (int i = 0; i< imaxOut; i++)
		{
			indexOut = i * planes + j * strideOut; //Ŀ������[i, j]

		

												 //��֤������Ч����������
			x = max(x, 0);
			x = min(x, imaxIn - 1);
			y = max(y, 0);
			y = min(y, jmaxIn - 1);
			indexIn = x * planes + y * strideIn; //Դ����[x, y]
												 //Ϊ�˼����ˣ�����Ĭ�ϰ�ͼ����RGB��ʽ��ʵ���ϲ�Ӧ��������
			r = pDataOut[indexOut];
			g = pDataOut[indexOut + 1];
			b = pDataOut[indexOut + 2];
			a = pDataOut[indexOut + 3];

			//if (abs((int)r- (int)g)>30 || abs((int)b - (int)g)>30 || abs((int)b - (int)r)>30 ) {

		//	pDataOut[indexOut] = 255;	//Red  
		//	pDataOut[indexOut + 1] = 0;	//Green 
		//	pDataOut[indexOut + 2] = 0;	//Blue
		//	pDataOut[indexOut + 3] = 0;	//A
			//}
			//else {
			float gray = ( 299*r +  587*g + 114*b + 500) / 1000;
			uint8 p = 60;
			if(gray<180){
		//	if (r<p&&b<p&&g<p) {
			//	pDataOut[indexOut] = 0;	//Red  
			//	pDataOut[indexOut + 1] = 0;	//Green 
			//	pDataOut[indexOut + 2] = 0;	//Blue
				pDataOut[indexOut + 3] = 255;
			}
			else {
				pDataOut[indexOut] = 255;	//Red  
				pDataOut[indexOut + 1] = 0;	//Green 
				pDataOut[indexOut + 2] = 0;	//Blue
				pDataOut[indexOut + 3] = 0;
			}
		//	pDataOut[indexOut] = r;	//Red  
		//	pDataOut[indexOut + 1] = g;	//Green 
		//	pDataOut[indexOut + 2] = b;	//Blue
		//	pDataOut[indexOut + 3] = a;
				
			//	pDataOut[indexOut + 3] = 255;	//A
		//	}
			//pDataOut[indexOut + 3] = 255;	//A
		//	pDataOut[indexOut] = (uint8)((pDataIn[indexIn] * (100 - o1) + r*o1) / 100);	//Red  
	//		pDataOut[indexOut + 1] = (uint8)((pDataIn[indexIn + 1] * (100 - o2) + g*o2) / 100);	//Green 
		//	pDataOut[indexOut + 2] = (uint8)((pDataIn[indexIn + 2] * (100 - o3) + b*o3) / 100);	//Blue
		}
	}

	//�ж��Ƿ��Ѿ��������
	if (m_TileOut.right >= gFilterRecord->filterRect.right && m_TileOut.bottom >= gFilterRecord->filterRect.bottom)
	{
		//�������
		ZeroPsRect(&gFilterRecord->inRect);
		ZeroPsRect(&gFilterRecord->outRect);
		ZeroPsRect(&gFilterRecord->maskRect);
		return;
	}
	//������һ��tile
	if (m_TileOut.right < gFilterRecord->filterRect.right)
	{
		//�����ƶ�һ��
		m_TileOut.left = m_TileOut.right;
		m_TileOut.right = min(m_TileOut.right + TILESIZE, gFilterRecord->filterRect.right);

	}
	else
	{
		//���»��в��ص����״�
		m_TileOut.left = gFilterRecord->filterRect.left;
		m_TileOut.right = min(m_TileOut.left + TILESIZE, gFilterRecord->filterRect.right);
		m_TileOut.top = m_TileOut.bottom;
		m_TileOut.bottom = min(m_TileOut.bottom + TILESIZE, gFilterRecord->filterRect.bottom);
	}

	//������һ��TileIn����TileOut�Ļ�������������չdistance���룩
	m_TileIn.left = max(m_TileOut.left , gFilterRecord->filterRect.left);
	m_TileIn.top = max(m_TileOut.top , gFilterRecord->filterRect.top);
	m_TileIn.right = min(m_TileOut.right , gFilterRecord->filterRect.right);
	m_TileIn.bottom = min(m_TileOut.bottom , gFilterRecord->filterRect.bottom);

	//������һ������ľ��� InRect �� OutRect
	CopyPsRect(&m_TileIn, &gFilterRecord->inRect);
	CopyPsRect(&m_TileOut, &gFilterRecord->outRect);

	//����ȫ��ͨ����������Ϊinterleave�ֲ���
	gFilterRecord->inLoPlane = 0;
	gFilterRecord->inHiPlane = (gFilterRecord->planes - 1);;
	gFilterRecord->outLoPlane = 0;
	gFilterRecord->outHiPlane = (gFilterRecord->planes - 1);
	return;
	
	}

/*****************************************************************************/

/* Do any necessary clean-up. */

void DoFinish (GPtr globals)
{	
	WriteScriptParams (globals); // writes our parameters to the scripting system
}

/*******************************************************************/
void PopulateColorServicesInfo (GPtr globals, ColorServicesInfo *csinfo)
{
	CSInitInfo(csinfo);
	// now populate color services info with particulars:
	csinfo->sourceSpace = gColor;
	csinfo->resultSpace = gColorize;
	CSCopyColor(csinfo->colorComponents, gSourceColor);
	// on input, source color; on output, result color
}

//-------------------------------------------------------------------------------

// end ColorMunger.cpp
