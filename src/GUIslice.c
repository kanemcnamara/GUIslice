// =======================================================================
// GUIslice library
// - Calvin Hass
// - http://www.impulseadventure.com/elec/microsdl-sdl-gui.html
//
// - Version 0.6.2    (2016/11/19)
// =======================================================================
//
// The MIT License
//
// Copyright 2016 Calvin Hass
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =======================================================================


// GUIslice library
#include "GUIslice.h"
#include "GUIslice_ex.h"

// Include rendering driver
#include "GUIslice_drv_sdl1.h"

#include <stdio.h>

// Includes for tslib
#ifdef INC_TS
#include "tslib.h"
#endif


// Version definition
#define GUISLICE_VER "0.6.2"

// Debug flags
//#define DBG_LOG     // Enable debugging log output
//#define DBG_TOUCH   // Enable debugging of touch-presses



// ========================================================================

// ------------------------------------------------------------------------
// General Functions
// ------------------------------------------------------------------------

char* gslc_GetVer(gslc_tsGui* pGui)
{
  return (char*)GUISLICE_VER;
}

// May need to configure some environment variables
// depending on the driver being used. It can also
// be done via export commands within the shell (or init script).
// eg. export TSLIB_FBDEVICE=/dev/fb1
void gslc_InitEnv(char* acDevFb,char* acDevTouch)
{
  gslc_DrvInitEnv(acDevFb,acDevTouch);
}



bool gslc_Init(gslc_tsGui* pGui,gslc_tsPage* asPage,unsigned nMaxPage,gslc_tsFont* asFont,unsigned nMaxFont,gslc_tsView* asView,unsigned nMaxView)
{
  unsigned  nInd;
  
  // Initialize state
  pGui->nPageMax        = nMaxPage;
  pGui->nPageCnt        = 0;
  pGui->asPage          = asPage;

  pGui->pCurPage        = NULL;
  pGui->pCurPageCollect = NULL;

  // Initialize collection of fonts with user-supplied pointer
  pGui->asFont      = asFont;
  pGui->nFontMax    = nMaxFont;
  pGui->nFontCnt    = 0;
  for (nInd=0;nInd<(pGui->nFontMax);nInd++) {
    gslc_ResetFont(&(pGui->asFont[nInd]));
  }
 
  // Initialize temporary element
  gslc_ResetElem(&(pGui->sElemTmp));

  // Initialize collection of views with user-supplied pointer
  pGui->asView      = asView;
  pGui->nViewMax    = nMaxView;
  pGui->nViewCnt    = 0;
  pGui->nViewIndCur = GSLC_VIEW_IND_SCREEN;
  for (nInd=0;nInd<(pGui->nViewMax);nInd++) {
    gslc_ResetView(&(pGui->asView[nInd]));
  }
  
  // Last touch event
  pGui->nTouchLastX           = 0;
  pGui->nTouchLastY           = 0;
  pGui->nTouchLastPress       = 0;

  // Touchscreen library interface
  #ifdef INC_TS
  pGui->ts = NULL;
  #endif

  // Initialize the rendering driver
  return gslc_DrvInit(pGui);
}


void gslc_Quit(gslc_tsGui* pGui)
{
  // Close all elements and fonts
  gslc_GuiDestruct(pGui);

  // Close down SDL
  SDL_Quit();
}

// Main polling loop for GUIslice
void gslc_Update(gslc_tsGui* pGui)
{
  int       nTouchX,nTouchY;
  unsigned  nTouchPress;
  bool      bTouchEvent;
  
  // Poll for touchscreen presses
  // - Conditional compiling for tslib or selected driver mode
  #ifdef INC_TS  
  bTouchEvent = gslc_GetTsTouch(pGui,&nTouchX,&nTouchY,&nTouchPress);
  #else
  bTouchEvent = gslc_DrvGetTouch(pGui,&nTouchX,&nTouchY,&nTouchPress);
  #endif
  
  if (bTouchEvent) {
    // Track and handle the touch events
    // - Handle the events on the current page
    gslc_TrackTouch(pGui,pGui->pCurPageCollect,nTouchX,nTouchY,nTouchPress);
  }
  
  // Issue a timer tick to all pages
  int nPage;
  gslc_tsCollect* pCollect = NULL;
  for (nPage=0;nPage<pGui->nPageCnt;nPage++) {
    pCollect = &pGui->asPage[nPage].sCollect;
    gslc_CollectTick(pGui,pCollect);
  }
  
  // Perform any redraw required for current page
  gslc_PageRedrawGo(pGui);
}

// ------------------------------------------------------------------------
// Graphics General Functions
// ------------------------------------------------------------------------


bool gslc_IsInRect(int nSelX,int nSelY,gslc_Rect rRect)
{
  if ( (nSelX >= rRect.x) && (nSelX <= rRect.x+rRect.w) && 
     (nSelY >= rRect.y) && (nSelY <= rRect.y+rRect.h) ) {
    return true;
  } else {
    return false;
  }
}

bool gslc_IsInWH(gslc_tsGui* pGui,int nSelX,int nSelY,uint16_t nWidth,uint16_t nHeight)
{
  if ( (nSelX >= 0) && (nSelX <= nWidth-1) && 
     (nSelY >= 0) && (nSelY <= nHeight-1) ) {
    return true;
  } else {
    return false;
  }
}

// Ensure the coordinates are increasing from nX0->nX1 and nY0->nY1
// NOTE: UNUSED
void gslc_OrderCoord(int16_t* pnX0,int16_t* pnY0,int16_t* pnX1,int16_t* pnY1)
{
  int16_t  nTmp;
  if ((*pnX1) < (*pnX0)) {
    nTmp = (*pnX0);
    (*pnX0) = (*pnX1);
    (*pnX1) = nTmp;
  }
  if ((*pnY1) < (*pnY0)) {
    nTmp = (*pnY0);
    (*pnY0) = (*pnY1);
    (*pnY1) = nTmp;
  }
}

// ------------------------------------------------------------------------
// Graphics Primitive Functions
// ------------------------------------------------------------------------

// bMapEn specifies whether viewport remapping is enabled or not
// - This should be disabled if the caller has already performed the
//   the coordinate remapping.
void gslc_DrawSetPixel(gslc_tsGui* pGui,int16_t nX,int16_t nY,gslc_Color nCol,bool bMapEn)
{
  // Support viewport local coordinate remapping
  if (bMapEn) {
    if (pGui->nViewIndCur != GSLC_VIEW_IND_SCREEN) {
      gslc_ViewRemapPt(pGui,&nX,&nY);
    }
  }

  if (gslc_DrvScreenLock(pGui)) {
    uint32_t nColRaw = gslc_DrvAdaptColorRaw(pGui,nCol);    
    gslc_DrvDrawSetPixelRaw(pGui,nX,nY,nColRaw);
    gslc_DrvScreenUnlock(pGui);
  }   // gslc_DrvScreenLock
  gslc_PageFlipSet(pGui,true);
}

// Draw an arbitrary line using Bresenham's algorithm
// - Algorithm reference: https://rosettacode.org/wiki/Bitmap/Bresenham's_line_algorithm#C
void gslc_DrawLine(gslc_tsGui* pGui,int16_t nX0,int16_t nY0,int16_t nX1,int16_t nY1,gslc_Color nCol)
{
  // Support viewport local coordinate remapping
  if (pGui->nViewIndCur != GSLC_VIEW_IND_SCREEN) {
    gslc_ViewRemapPt(pGui,&nX0,&nY0);
    gslc_ViewRemapPt(pGui,&nX1,&nY1);
  }

#if (DRV_HAS_DRAW_LINE) 
  // Call optimized driver line drawing
  gslc_DrvDrawLine(pGui,nX0,nY0,nX1,nY1,nCol);
  
#else
  // Perform Bresenham's line algorithm
  int16_t nDX = abs(nX1-nX0);
  int16_t nDY = abs(nY1-nY0);

  int16_t nSX = (nX0 < nX1)? 1 : -1;
  int16_t nSY = (nY0 < nY1)? 1 : -1;
  int16_t nErr = ( (nDX>nDY)? nDX : -nDY )/2;
  int16_t nE2;

  // Check for degenerate cases
  // TODO: Need to test these optimizations
  if (nDX == 0) {
    if (nDY == 0) {
      return;
    } else if (nDY >= 0) {
      gslc_DrawLineV(pGui,nX0,nY0,nDY,nCol);
    } else {
      gslc_DrawLineV(pGui,nX1,nY1,-nDY,nCol);
    }
  } else if (nDY == 0) {
    if (nDX >= 0) {
      gslc_DrawLineH(pGui,nX0,nY0,nDX,nCol);
    } else {
      gslc_DrawLineH(pGui,nX1,nY1,-nDX,nCol);
    }
  }

  uint32_t nColRaw = gslc_DrvAdaptColorRaw(pGui,nCol);   
  if (gslc_DrvScreenLock(pGui)) {
    for (;;) {
      // Set the pixel
      gslc_DrvDrawSetPixelRaw(pGui,nX0,nY0,nColRaw);
      
      // Calculate next coordinates
      if ( (nX0 == nX1) && (nY0 == nY1) ) break;
      nE2 = nErr;
      if (nE2 > -nDX) { nErr -= nDY; nX0 += nSX; }
      if (nE2 <  nDY) { nErr += nDX; nY0 += nSY; }
    }
  }   // gslc_DrvScreenLock  
#endif
  
  gslc_PageFlipSet(pGui,true);  
  
}


void gslc_DrawLineH(gslc_tsGui* pGui,int16_t nX, int16_t nY, uint16_t nW,gslc_Color nCol)
{
  // Support viewport local coordinate remapping
  if (pGui->nViewIndCur != GSLC_VIEW_IND_SCREEN) {
    gslc_ViewRemapPt(pGui,&nX,&nY);
  }

  uint16_t nOffset;
  uint32_t nColRaw = gslc_DrvAdaptColorRaw(pGui,nCol);
  if (gslc_DrvScreenLock(pGui)) {
    for (nOffset=0;nOffset<nW;nOffset++) {
      gslc_DrvDrawSetPixelRaw(pGui,nX+nOffset,nY,nColRaw);    
    }
    gslc_DrvScreenUnlock(pGui);
  }   // gslc_DrvScreenLock
  gslc_PageFlipSet(pGui,true);  
}

void gslc_DrawLineV(gslc_tsGui* pGui,int16_t nX, int16_t nY, uint16_t nH,gslc_Color nCol)
{
  // Support viewport local coordinate remapping
  if (pGui->nViewIndCur != GSLC_VIEW_IND_SCREEN) {
    gslc_ViewRemapPt(pGui,&nX,&nY);
  }
  uint16_t nOffset;
  uint32_t nColRaw = gslc_DrvAdaptColorRaw(pGui,nCol);
  if (gslc_DrvScreenLock(pGui)) {
    for (nOffset=0;nOffset<nH;nOffset++) {
      gslc_DrvDrawSetPixelRaw(pGui,nX,nY+nOffset,nColRaw);    
    }
    gslc_DrvScreenUnlock(pGui);
  }   // gslc_DrvScreenLock
  gslc_PageFlipSet(pGui,true);
}


void gslc_DrawFrameRect(gslc_tsGui* pGui,gslc_Rect rRect,gslc_Color nCol)
{
  // Ensure dimensions are valid
  if ((rRect.w == 0) || (rRect.h == 0)) {
    return;
  }

  // Support viewport local coordinate remapping
  if (pGui->nViewIndCur != GSLC_VIEW_IND_SCREEN) {
    gslc_ViewRemapRect(pGui,&rRect);
  }

#if (DRV_HAS_DRAW_RECT_FRAME)
  // Call optimized driver implementation
  gslc_DrvDrawFrameRect(pGui,rRect,nCol);
#else
  // Emulate rect frame with four lines
  int16_t  nX,nY;
  uint16_t  nH,nW;
  nX = rRect.x;
  nY = rRect.y;
  nW = rRect.w;
  nH = rRect.h;
  gslc_DrawLineH(pGui,nX,nY,nW-1,nCol);                 // Top
  gslc_DrawLineH(pGui,nX,(int16_t)(nY+nH-1),nW-1,nCol); // Bottom
  gslc_DrawLineV(pGui,nX,nY,nH-1,nCol);                 // Left
  gslc_DrawLineV(pGui,(int16_t)(nX+nW-1),nY,nH-1,nCol); // Right
#endif
  
  gslc_PageFlipSet(pGui,true);  
}

void gslc_DrawFillRect(gslc_tsGui* pGui,gslc_Rect rRect,gslc_Color nCol)
{
  // Ensure dimensions are valid
  if ((rRect.w == 0) || (rRect.h == 0)) {
    return;
  }

  // Support viewport local coordinate remapping
  if (pGui->nViewIndCur != GSLC_VIEW_IND_SCREEN) {
    gslc_ViewRemapRect(pGui,&rRect);
  }

#if (DRV_HAS_DRAW_RECT_FILL)
  // Call optimized driver implementation
  gslc_DrvDrawFillRect(pGui,rRect,nCol);
#else
  // TODO: Emulate it with individual line draws
#endif
  
  gslc_PageFlipSet(pGui,true);    
}


// Expand or contract a rectangle in width and/or height (equal
// amounts on both side), based on the centerpoint of the rectangle.
gslc_Rect gslc_ExpandRect(gslc_Rect rRect,int16_t nExpandW,int16_t nExpandH)
{
  gslc_Rect  rNew = {0,0,0,0};

  // Detect error case of contracting region too far
  if (rRect.w + (2*nExpandW) < 0) {
    fprintf(stderr,"ERROR: ExpandRect(%d,%d) contracts too far",nExpandW,nExpandH);
    return rNew;
  }
  if (rRect.w + (2*nExpandW) < 0) {
    fprintf(stderr,"ERROR: ExpandRect(%d,%d) contracts too far",nExpandW,nExpandH);
    return rNew;
  }

  // Adjust the new width/height
  // Note that the overall width/height changes by a factor of
  // two since we are applying the adjustment on both sides (ie.
  // top/bottom or left/right) equally.
  rNew.w = rRect.w + (2*nExpandW);
  rNew.h = rRect.h + (2*nExpandH);

  // Adjust the rectangle coordinate to allow for new dimensions
  // Note that this moves the coordinate in the opposite
  // direction of the expansion/contraction.
  rNew.x = rRect.x - nExpandW;
  rNew.y = rRect.y - nExpandH;

  return rNew;
}


// Draw a circle using midpoint circle algorithm
// - Algorithm reference: https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
void gslc_DrawFrameCircle(gslc_tsGui* pGui,int16_t nMidX,int16_t nMidY,
  uint16_t nRadius,gslc_Color nCol)
{
  // Support viewport local coordinate remapping
  if (pGui->nViewIndCur != GSLC_VIEW_IND_SCREEN) {
    gslc_ViewRemapPt(pGui,&nMidX,&nMidY);
  }
  uint32_t nColRaw = gslc_DrvAdaptColorRaw(pGui,nCol);
  
  int nX    = nRadius;
  int nY    = 0;
  int nErr  = 0;
  
  if (gslc_DrvScreenLock(pGui)) {
    while (nX >= nY)
    {
      gslc_DrvDrawSetPixelRaw(pGui,nMidX + nX, nMidY + nY,nColRaw);
      gslc_DrvDrawSetPixelRaw(pGui,nMidX + nY, nMidY + nX,nColRaw);
      gslc_DrvDrawSetPixelRaw(pGui,nMidX - nY, nMidY + nX,nColRaw);
      gslc_DrvDrawSetPixelRaw(pGui,nMidX - nX, nMidY + nY,nColRaw);
      gslc_DrvDrawSetPixelRaw(pGui,nMidX - nX, nMidY - nY,nColRaw);
      gslc_DrvDrawSetPixelRaw(pGui,nMidX - nY, nMidY - nX,nColRaw);
      gslc_DrvDrawSetPixelRaw(pGui,nMidX + nY, nMidY - nX,nColRaw);
      gslc_DrvDrawSetPixelRaw(pGui,nMidX + nX, nMidY - nY,nColRaw);

      nY    += 1;
      nErr  += 1 + 2*nY;
      if (2*(nErr-nX) + 1 > 0)
      {
          nX -= 1;
          nErr += 1 - 2*nX;
      }
    } // while

    gslc_DrvScreenUnlock(pGui);
  }   // gslc_DrvScreenLock
  
  gslc_PageFlipSet(pGui,true);
}



// -----------------------------------------------------------------------
// Font Functions
// -----------------------------------------------------------------------

bool gslc_FontAdd(gslc_tsGui* pGui,int nFontId,const char* acFontName,unsigned nFontSz)
{
  if (pGui->nFontCnt+1 >= (pGui->nFontMax)) {
    fprintf(stderr,"ERROR: FontAdd() added too many fonts\n");
    return false;
  } else {
    TTF_Font*   pFont;
    pFont = TTF_OpenFont(acFontName,nFontSz);
    if (pFont == NULL) {
      fprintf(stderr,"ERROR: TTF_OpenFont(%s) failed\n",acFontName);
      return false;
    }
    pGui->asFont[pGui->nFontCnt].pvFont = (void*)pFont;
    pGui->asFont[pGui->nFontCnt].nId = nFontId;
    pGui->nFontCnt++;  
    return true;
  }
}


void* gslc_FontGet(gslc_tsGui* pGui,int nFontId)
{
  unsigned  nFontInd;
  for (nFontInd=0;nFontInd<pGui->nFontCnt;nFontInd++) {
    if (pGui->asFont[nFontInd].nId == nFontId) {
      return pGui->asFont[nFontInd].pvFont;
    }
  }
  return NULL;
}




// ------------------------------------------------------------------------
// Page Functions
// ------------------------------------------------------------------------


void gslc_PageAdd(gslc_tsGui* pGui,int nPageId,gslc_tsElem* psElem,unsigned nMaxElem)
{
  gslc_tsPage*  pPage = &pGui->asPage[pGui->nPageCnt];

  gslc_CollectReset(&pPage->sCollect,psElem,nMaxElem);
  
  // Assign the requested Page ID
  pPage->nPageId = nPageId;
  
  // Increment the page count
  pGui->nPageCnt++;
  
  // Default the page pointer to the first page we create
  if (gslc_GetPageCur(pGui) == GSLC_PAGE_NONE) {
    gslc_SetPageCur(pGui,nPageId);
  }
  
  // Force the page to redraw
  gslc_PageRedrawSet(pGui,true);

}

int gslc_GetPageCur(gslc_tsGui* pGui)
{
  if (pGui->pCurPage == NULL) {
    return GSLC_PAGE_NONE;
  }
  return pGui->pCurPage->nPageId;
}


void gslc_SetPageCur(gslc_tsGui* pGui,int nPageId)
{
  int nPageSaved = GSLC_PAGE_NONE;
  if (pGui->pCurPage != NULL) {
    nPageSaved = pGui->pCurPage->nPageId;
  }
  
  // Find the page
  gslc_tsPage* pPage = gslc_PageFindById(pGui,nPageId);
  if (pPage == NULL) {
    fprintf(stderr,"ERROR: SetPageCur() can't find page (ID=%d)\n",nPageId);
    exit(1);
  }
  
  // Save a reference to the selected page
  pGui->pCurPage = pPage;
  
  // Save a reference to the selected page's element collection
  pGui->pCurPageCollect = &pPage->sCollect;
  
  // A change of page should always force a future redraw
  if (nPageSaved != nPageId) {
    gslc_PageRedrawSet(pGui,true);
  }
}


// Adjust the flag that indicates whether the entire page
// requires a redraw.
void gslc_PageRedrawSet(gslc_tsGui* pGui,bool bRedraw)
{
  pGui->pCurPage->bPageNeedRedraw = bRedraw;
}

bool gslc_PageRedrawGet(gslc_tsGui* pGui)
{
  return pGui->pCurPage->bPageNeedRedraw;
}

// Check the redraw flag on all elements on the current page and update
// the redraw status if additional redraws are required (or the
// entire page should be marked as requiring redraw).
// - The typical case for this being required is when an element
//   requires redraw but it is marked as being transparent. Therefore,
//   the lower level elements should be redrawn.
// - For now, just mark the entire page as requiring redraw.
// TODO: Determine which elements underneath should be redrawn based
//       on the region exposed by the transparent element.
void gslc_PageRedrawCalc(gslc_tsGui* pGui)
{
  int               nInd;
  gslc_tsElem*  pElem = NULL;
  for (nInd=GSLC_IND_FIRST;nInd<pGui->pCurPageCollect->nElemCnt;nInd++) {
    pElem = &(pGui->pCurPageCollect->asElem[nInd]);
    if (pElem->bNeedRedraw) {
      // Is the element transparent?
      if (!pElem->bFillEn) {
        // For now, mark the entire page as requiring redraw
        gslc_PageRedrawSet(pGui,true);
        // No need to check any more elements
        break;
      }
    }
  }
}

// Redraw the active page
// - If the page has been marked as needing redraw, then all
//   elements are rendered
// - If the page has not been marked as needing redraw then only
//   the elements that have been marked as needing redraw
//   are rendered.
void gslc_PageRedrawGo(gslc_tsGui* pGui)
{ 
  // Update any page redraw status that may be required
  // - Note that this routine handles cases where an element
  //   marked as requiring update is semi-transparent which can
  //   cause other elements to be redrawn as well.
  gslc_PageRedrawCalc(pGui);

  // Determine final state of full-page redraw
  bool  bPageRedraw = gslc_PageRedrawGet(pGui);

  // If a full page redraw is required, then start by
  // redrawing the background.
  // NOTE:
  // - It would be cleaner if we could treat the background
  //   layer like any other element (and hence check for its
  //   need-redraw status).
  // - For now, assume background doesn't need update except
  //   if the entire page is to be redrawn
  //   TODO: Fix this assumption (either add specific flag
  //         for bBkgndNeedRedraw or make the background just
  //         another element).
  if (bPageRedraw) {
    gslc_DrvPasteSurface(pGui,0,0,pGui->pvSurfBkgnd,pGui->pvSurfScreen);
  }
    
  // Draw other elements (as needed)
  gslc_CollectRedraw(pGui,pGui->pCurPageCollect,bPageRedraw);

  // Clear the page redraw flag
  gslc_PageRedrawSet(pGui,false);
  
  // Page flip the entire screen
  // - TODO: We could also call Update instead of Flip as that would
  //         limit the region to refresh.
  gslc_PageFlipGo(pGui);
  
}


void gslc_PageFlipSet(gslc_tsGui* pGui,bool bNeeded)
{
  pGui->pCurPage->bPageNeedFlip = bNeeded;
}

bool gslc_PageFlipGet(gslc_tsGui* pGui)
{
  return pGui->pCurPage->bPageNeedFlip;
}

void gslc_PageFlipGo(gslc_tsGui* pGui)
{
  if (pGui->pCurPage->bPageNeedFlip) {
    gslc_DrvPageFlipNow(pGui);
    
    // Indicate that page flip is no longer required
    gslc_PageFlipSet(pGui,false);
  }
}


gslc_tsPage* gslc_PageFindById(gslc_tsGui* pGui,int nPageId)
{
  int nInd;
  
  // Loop through list of pages
  // Return pointer to page
  gslc_tsPage*  pFoundPage = NULL;
  for (nInd=0;nInd<pGui->nPageMax;nInd++) {
    if (pGui->asPage[nInd].nPageId == nPageId) {
      pFoundPage = &pGui->asPage[nInd];
      break;
    }
  }
  
  // Error handling: if not found, make this a fatal error
  // as it shows a serious config error and continued operation
  // is not viable.
  if (pFoundPage == NULL) {
    fprintf(stderr,"ERROR: PageGet() could not find page (ID=%d)",nPageId);
    exit(1);
  }
  
  return pFoundPage;
}

gslc_tsElem* gslc_PageFindElemById(gslc_tsGui* pGui,int nPageId,int nElemId)
{
  gslc_tsPage*  pPage = NULL;
  gslc_tsElem*  pElem = NULL;

  // Get the page
  pPage = gslc_PageFindById(pGui,nPageId);
  if (pPage == NULL) {
    fprintf(stderr,"ERROR: PageFindElemById() can't find page (ID=%d)\n",nPageId);
    exit(1);
  }
  // Find the element in the page's element collection
  pElem = gslc_CollectFindElemById(&pPage->sCollect,nElemId);
  return pElem;
}


// ------------------------------------------------------------------------
// Element General Functions
// ------------------------------------------------------------------------


int gslc_ElemGetId(gslc_tsElem* pElem)
{
  return pElem->nId;
}


// ------------------------------------------------------------------------
// Element Creation Functions
// ------------------------------------------------------------------------


gslc_tsElem* gslc_ElemCreateTxt(gslc_tsGui* pGui,int nElemId,int nPage,gslc_Rect rElem,
  const char* pStr,int nFontId)
{
  gslc_tsElem   sElem;
  gslc_tsElem*  pElem = NULL;
  sElem = gslc_ElemCreate(pGui,nElemId,nPage,GSLC_TYPE_TXT,rElem,pStr,nFontId);
  sElem.colElemFill       = GSLC_COL_BLACK;
  sElem.colElemFillGlow   = GSLC_COL_BLACK;
  sElem.colElemFrame      = GSLC_COL_BLACK;
  sElem.colElemFrameGlow  = GSLC_COL_BLACK;
  sElem.colElemText       = GSLC_COL_YELLOW;
  sElem.colElemTextGlow   = GSLC_COL_YELLOW;
  sElem.bFillEn           = true;
  sElem.eTxtAlign         = GSLC_ALIGN_MID_LEFT;
  if (nPage != GSLC_PAGE_NONE) {
    pElem = gslc_ElemAdd(pGui,nPage,&sElem);
    return pElem;
  } else {
    // Save as temporary element
    pGui->sElemTmp = sElem;
    return &(pGui->sElemTmp);      
  }
}


gslc_tsElem* gslc_ElemCreateBtnTxt(gslc_tsGui* pGui,int nElemId,int nPage,
  gslc_Rect rElem,const char* acStr,int nFontId,GSLC_CB_TOUCH cbTouch)
{
  gslc_tsElem   sElem;
  gslc_tsElem*  pElem = NULL;
  
  // Ensure the Font is loaded
  if (gslc_FontGet(pGui,nFontId) == NULL) {
    fprintf(stderr,"ERROR: ElemCreateBtnTxt(ID=%d): Font(ID=%d) not loaded\n",nElemId,nFontId);
    return NULL;
  }

  sElem = gslc_ElemCreate(pGui,nElemId,nPage,GSLC_TYPE_BTN,rElem,acStr,nFontId);
  sElem.colElemFill       = GSLC_COL_BLUE_DK4;
  sElem.colElemFillGlow   = GSLC_COL_BLUE_DK1;
  sElem.colElemFrame      = GSLC_COL_BLUE_DK2;
  sElem.colElemFrameGlow  = GSLC_COL_BLUE_DK2;
  sElem.colElemText       = GSLC_COL_WHITE;
  sElem.colElemTextGlow   = GSLC_COL_WHITE;
  sElem.bFrameEn          = true;
  sElem.bFillEn           = true;
  sElem.bClickEn          = true;
  sElem.bGlowEn           = true;
  sElem.pfuncXTouch       = cbTouch;
  if (nPage != GSLC_PAGE_NONE) {
    pElem = gslc_ElemAdd(pGui,nPage,&sElem);
    return pElem;
  } else {
    // Save as temporary element
    pGui->sElemTmp = sElem;
    return &(pGui->sElemTmp);       
  }
}

gslc_tsElem* gslc_ElemCreateBtnImg(gslc_tsGui* pGui,int nElemId,int nPage,
  gslc_Rect rElem,const char* acImg,const char* acImgSel,GSLC_CB_TOUCH cbTouch)
{
  gslc_tsElem   sElem;
  gslc_tsElem*  pElem = NULL;
  sElem = gslc_ElemCreate(pGui,nElemId,nPage,GSLC_TYPE_BTN,rElem,"",GSLC_FONT_NONE);
  sElem.colElemFill       = GSLC_COL_BLACK;
  sElem.colElemFillGlow   = GSLC_COL_BLACK;
  sElem.colElemFrame      = GSLC_COL_BLUE_DK2;
  sElem.colElemFrameGlow  = GSLC_COL_BLUE_DK2;
  sElem.bFrameEn          = false;
  sElem.bFillEn           = false;
  sElem.bClickEn          = true;
  sElem.bGlowEn           = true;  
  sElem.pfuncXTouch       = cbTouch;  
  gslc_ElemSetImage(pGui,&sElem,acImg,acImgSel);
  if (nPage != GSLC_PAGE_NONE) {
    pElem = gslc_ElemAdd(pGui,nPage,&sElem);
    return pElem;
  } else {
    // Save as temporary element
    pGui->sElemTmp = sElem;
    return &(pGui->sElemTmp);     
  }
}


gslc_tsElem* gslc_ElemCreateBox(gslc_tsGui* pGui,int nElemId,int nPage,gslc_Rect rElem)
{
  gslc_tsElem   sElem;
  gslc_tsElem*  pElem = NULL;
  sElem = gslc_ElemCreate(pGui,nElemId,nPage,GSLC_TYPE_BOX,rElem,NULL,GSLC_FONT_NONE);
  sElem.colElemFill       = GSLC_COL_BLACK;
  sElem.colElemFillGlow   = GSLC_COL_BLACK;
  sElem.colElemFrame      = GSLC_COL_GRAY;
  sElem.colElemFrameGlow  = GSLC_COL_GRAY;
  sElem.bFillEn           = true;
  sElem.bFrameEn          = true;
  if (nPage != GSLC_PAGE_NONE) {
    pElem = gslc_ElemAdd(pGui,nPage,&sElem);  
    return pElem;
  } else {
    // Save as temporary element
    pGui->sElemTmp = sElem;
    return &(pGui->sElemTmp);     
  }
}


gslc_tsElem* gslc_ElemCreateImg(gslc_tsGui* pGui,int nElemId,int nPage,
  gslc_Rect rElem,const char* acImg)
{
  gslc_tsElem   sElem;
  gslc_tsElem*  pElem = NULL;
  sElem = gslc_ElemCreate(pGui,nElemId,nPage,GSLC_TYPE_BOX,rElem,"",GSLC_FONT_NONE);
  sElem.bFrameEn        = false;
  sElem.bFillEn         = false;
  sElem.bClickEn        = false;
  gslc_ElemSetImage(pGui,&sElem,acImg,acImg);
  if (nPage != GSLC_PAGE_NONE) {
    pElem = gslc_ElemAdd(pGui,nPage,&sElem);
    return pElem;
  } else {
    // Save as temporary element
    pGui->sElemTmp = sElem;
    return &(pGui->sElemTmp);     
  }
}



// ------------------------------------------------------------------------
// Element Drawing Functions
// ------------------------------------------------------------------------


// Draw an element to the active display
// - Element is referenced by page ID and element ID
// - This routine is typically called by user code for custom
//   drawing callbacks
void gslc_ElemDraw(gslc_tsGui* pGui,int nPageId,int nElemId)
{
  gslc_tsElem* pElem = gslc_PageFindElemById(pGui,nPageId,nElemId);
  gslc_ElemDrawByRef(pGui,pElem);
}

// Draw an element to the active display
// - Element is referenced by an element pointer
// - TODO: Handle GSLC_TYPE_BKGND
bool gslc_ElemDrawByRef(gslc_tsGui* pGui,gslc_tsElem* pElem)
{
  if ((pGui == NULL) || (pElem == NULL)) {
    fprintf(stderr,"ERROR: ElemDrawByRef() called with NULL ptr\n");
    return false;
  }    
  
  // --------------------------------------------------------------------------
  // Custom drawing
  // --------------------------------------------------------------------------
  
  // Handle any extended element types
  // - If the pfuncXDraw callback is defined, then let the callback
  //   function supersede all default handling here
  // - Note that the end of the callback function is expected
  //   to clear the redraw flag
  if (pElem->pfuncXDraw != NULL) {
    (*pElem->pfuncXDraw)((void*)(pGui),(void*)(pElem));
    return true;
  }  
  
  // --------------------------------------------------------------------------
  // Init for default drawing
  // --------------------------------------------------------------------------
  
  bool      bGlowEn,bGlowing;
  int       nElemX,nElemY;
  
  nElemX    = pElem->rElem.x;
  nElemY    = pElem->rElem.y;
  bGlowEn   = pElem->bGlowEn;  // Does the element support glow state?
  bGlowing  = pElem->bGlowing; // Is the element currently glowing?
  
  // --------------------------------------------------------------------------
  // Background
  // --------------------------------------------------------------------------
  
  // Fill in the background
  // - This also changes the fill color if selected and glow state is enabled
  if (pElem->bFillEn) {
    if (bGlowEn && bGlowing) {
      gslc_DrawFillRect(pGui,pElem->rElem,pElem->colElemFillGlow);
    } else {
      gslc_DrawFillRect(pGui,pElem->rElem,pElem->colElemFill);
    }
  } else {
    // TODO: If unfilled, then we might need
    // to redraw the background layer(s)
  }

  // --------------------------------------------------------------------------
  // Frame
  // --------------------------------------------------------------------------

  // Frame the region
  #ifdef DBG_FRAME
  // For debug purposes, draw a frame around every element
  gslc_DrawFrameRect(pGui,sElem.rElem,GSLC_COL_GRAY_DK);
  #else
  if (pElem->bFrameEn) {
    gslc_DrawFrameRect(pGui,pElem->rElem,pElem->colElemFrame);
  }
  #endif

  
  // --------------------------------------------------------------------------
  // Image overlays
  // --------------------------------------------------------------------------
  
  // Draw any images associated with element
  if (pElem->pvSurfNorm != NULL) {
    if ((bGlowEn && bGlowing) && (pElem->pvSurfGlow != NULL)) {
      gslc_DrvPasteSurface(pGui,nElemX,nElemY,pElem->pvSurfGlow,pGui->pvSurfScreen);
    } else {
      gslc_DrvPasteSurface(pGui,nElemX,nElemY,pElem->pvSurfNorm,pGui->pvSurfScreen);
    }
  }

  // --------------------------------------------------------------------------
  // Text overlays
  // --------------------------------------------------------------------------
 
  // Overlay the text
  bool    bRenderTxt = true;
  bRenderTxt &= (pElem->acStr[0] != '\0');
  if (bRenderTxt) {
#if (DRV_HAS_DRAW_TEXT)    
    // Call the driver text rendering routine
    gslc_DrvDrawTxt(pGui,pElem);
#else
    // No text support in driver, so skip
#endif
  }

  // Mark the element as no longer requiring redraw
  gslc_ElemSetRedraw(pElem,false);
  
  return true;
}



// ------------------------------------------------------------------------
// Element Update Functions
// ------------------------------------------------------------------------

void gslc_ElemSetFillEn(gslc_tsElem* pElem,bool bFillEn)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetFillEn() called with NULL ptr\n");
    return;
  }
  pElem->bFillEn          = bFillEn;  
  gslc_ElemSetRedraw(pElem,true); 
}


void gslc_ElemSetFrameEn(gslc_tsElem* pElem,bool bFrameEn)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetFrameEn() called with NULL ptr\n");
    return;
  }
  pElem->bFrameEn         = bFrameEn;  
  gslc_ElemSetRedraw(pElem,true); 
}


void gslc_ElemSetCol(gslc_tsElem* pElem,gslc_Color colFrame,gslc_Color colFill,gslc_Color colFillGlow)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetCol() called with NULL ptr\n");
    return;
  }  
  pElem->colElemFrame     = colFrame;
  pElem->colElemFill      = colFill;
  pElem->colElemFillGlow      = colFillGlow;
  gslc_ElemSetRedraw(pElem,true); 
}

void gslc_ElemSetGlowCol(gslc_tsElem* pElem,gslc_Color colFrameGlow,gslc_Color colFillGlow,gslc_Color colTxtGlow)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetColGlow() called with NULL ptr\n");
    return;
  }  
  pElem->colElemFrameGlow   = colFrameGlow;
  pElem->colElemFillGlow    = colFillGlow;
  pElem->colElemTextGlow    = colTxtGlow;
  gslc_ElemSetRedraw(pElem,true); 
}

void gslc_ElemSetGroup(gslc_tsElem* pElem,int nGroupId)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetGroup() called with NULL ptr\n");
    return;
  }    
  pElem->nGroup           = nGroupId;
}

int gslc_ElemGetGroup(gslc_tsElem* pElem)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemGetGroup() called with NULL ptr\n");
    return GSLC_GROUP_ID_NONE;
  }    
  return pElem->nGroup;  
}


void gslc_ElemSetTxtAlign(gslc_tsElem* pElem,unsigned nAlign)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetTxtAlign() called with NULL ptr\n");
    return;
  }    
  pElem->eTxtAlign        = nAlign;
  gslc_ElemSetRedraw(pElem,true);  
}

void gslc_ElemSetTxtMargin(gslc_tsElem* pElem,unsigned nMargin)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetTxtMargin() called with NULL ptr\n");
    return;
  }    
  pElem->nTxtMargin        = nMargin;
  gslc_ElemSetRedraw(pElem,true);  
}

void gslc_ElemSetTxtStr(gslc_tsElem* pElem,const char* pStr)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetTxtStr() called with NULL ptr\n");
    return;
  }    
  strncpy(pElem->acStr,pStr,GSLC_ELEM_STRLEN_MAX-1);
  pElem->acStr[GSLC_ELEM_STRLEN_MAX-1] = '\0';  // Force termination
  gslc_ElemSetRedraw(pElem,true);
}

void gslc_ElemSetTxtCol(gslc_tsElem* pElem,gslc_Color colVal)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetTxtCol() called with NULL ptr\n");
    return;
  }    
  pElem->colElemText      = colVal;
  pElem->colElemTextGlow  = colVal; // Default to same color for glowing state
  gslc_ElemSetRedraw(pElem,true); 
}

void gslc_ElemUpdateFont(gslc_tsGui* pGui,gslc_tsElem* pElem,int nFontId)
{
  if ((pGui == NULL) || (pElem == NULL)) {
    fprintf(stderr,"ERROR: ElemUpdateFont() called with NULL ptr\n");
    return;
  }    
  pElem->pvTxtFont = gslc_FontGet(pGui,nFontId);
  gslc_ElemSetRedraw(pElem,true);
}

void gslc_ElemSetRedraw(gslc_tsElem* pElem,bool bRedraw)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetRedraw() called with NULL ptr\n");
    return;
  }    
  pElem->bNeedRedraw      = bRedraw;
  
  // Now propagate up the element hierarchy
  // (eg. in case of compound elements)
  if (pElem->pElemParent != NULL) {
    gslc_ElemSetRedraw(pElem->pElemParent,bRedraw);
  }
}

void gslc_ElemSetGlow(gslc_tsElem* pElem,bool bGlowing)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetGlow() called with NULL ptr\n");
    return;
  }    
  pElem->bGlowing         = bGlowing;
  gslc_ElemSetRedraw(pElem,true);
}

bool gslc_ElemGetGlow(gslc_tsElem* pElem)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemGetGlow() called with NULL ptr\n");
    return false;
  }    
  return pElem->bGlowing;
}

void gslc_ElemSetGlowEn(gslc_tsElem* pElem,bool bGlowEn)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemSetGlowEn() called with NULL ptr\n");
    return;
  }    
  pElem->bGlowEn         = bGlowEn;
  gslc_ElemSetRedraw(pElem,true);
}

bool gslc_ElemGetGlowEn(gslc_tsElem* pElem)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemGetGlowEn() called with NULL ptr\n");
    return false;
  }    
  return pElem->bGlowEn;
}

void gslc_ElemSetStyleFrom(gslc_tsElem* pElemSrc,gslc_tsElem* pElemDest)
{
  if ((pElemSrc == NULL) || (pElemDest == NULL)) {
    fprintf(stderr,"ERROR: ElemSetStyleFrom() called with NULL ptr\n");
    return;
  }

  // nId
  // nType
  // rElem
  pElemDest->nGroup           = pElemSrc->nGroup;
  // bValid
  pElemDest->bGlowEn          = pElemSrc->bGlowEn;
  pElemDest->bGlowing         = pElemSrc->bGlowing;
  pElemDest->pvSurfNorm       = pElemSrc->pvSurfNorm;
  pElemDest->pvSurfGlow       = pElemSrc->pvSurfGlow;
  
  pElemDest->bClickEn         = pElemSrc->bClickEn;
  pElemDest->bFrameEn         = pElemSrc->bFrameEn;
  pElemDest->bFillEn          = pElemSrc->bFillEn;
  
  pElemDest->colElemFill      = pElemSrc->colElemFill;
  pElemDest->colElemFillGlow  = pElemSrc->colElemFillGlow;
  pElemDest->colElemFrame     = pElemSrc->colElemFrame;
  pElemDest->colElemFrameGlow = pElemSrc->colElemFrameGlow;

  // bNeedRedraw

  pElemDest->pElemParent      = pElemSrc->pElemParent;
  
  // acStr[GSLC_ELEM_STRLEN_MAX]
  pElemDest->colElemText      = pElemSrc->colElemText;
  pElemDest->colElemTextGlow  = pElemSrc->colElemTextGlow; 
  pElemDest->eTxtAlign        = pElemSrc->eTxtAlign;
  pElemDest->nTxtMargin       = pElemSrc->nTxtMargin;
  pElemDest->pvTxtFont        = pElemSrc->pvTxtFont;

  // pXData
  
  pElemDest->pfuncXDraw     = pElemSrc->pfuncXDraw;
  pElemDest->pfuncXTouch    = pElemSrc->pfuncXTouch;
  pElemDest->pfuncXTick     = pElemSrc->pfuncXTick;
   
  gslc_ElemSetRedraw(pElemDest,true); 
}


void gslc_ElemSetDrawFunc(gslc_tsElem* pElem,GSLC_CB_DRAW funcCb)
{
  if ((pElem == NULL) || (funcCb == NULL)) {
    fprintf(stderr,"ERROR: ElemSetDrawFunc() called with NULL ptr\n");
    return;
  }    
  pElem->pfuncXDraw       = funcCb;
  gslc_ElemSetRedraw(pElem,true);   
}

void gslc_ElemSetTickFunc(gslc_tsElem* pElem,GSLC_CB_TICK funcCb)
{
  if ((pElem == NULL) || (funcCb == NULL)) {
    fprintf(stderr,"ERROR: ElemSetTickFunc() called with NULL ptr\n");
    return;
  }    
  pElem->pfuncXTick       = funcCb; 
}


// ------------------------------------------------------------------------
// Viewport Functions
// ------------------------------------------------------------------------

// TODO: Consider changing return value to tsView*?
int gslc_ViewCreate(gslc_tsGui* pGui,int nViewId,gslc_Rect rView,unsigned nOriginX,unsigned nOriginY)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ViewCreate() called with NULL ptr\n");
    return GSLC_VIEW_ID_NONE;
  }      
  if (pGui->nViewCnt+1 >= (pGui->nViewMax)) {
    fprintf(stderr,"ERROR: ViewCreate() too many views\n");
    return GSLC_VIEW_ID_NONE;
  }
  // Add the element to the internal array
  gslc_tsView   sView;
  sView.nId         = nViewId;
  sView.rView       = rView;
  sView.nOriginX    = nOriginX;
  sView.nOriginY    = nOriginY;
  pGui->asView[pGui->nViewCnt] = sView;
  pGui->nViewCnt++;

  return nViewId;
}

bool gslc_ViewSetOrigin(gslc_tsGui* pGui,int nViewId,unsigned nOriginX,unsigned nOriginY)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ViewSetOrigin() called with NULL ptr\n");
    return false;
  }      
  // Find View
  int nViewInd = gslc_ViewFindIndFromId(pGui,nViewId);
  // If GSLC_VIEW_ID_SCREEN is passed, then it will be ignored here
  if (nViewInd == GSLC_VIEW_IND_NONE) {
    return false;
  }
  pGui->asView[nViewInd].nOriginX = nOriginX;
  pGui->asView[nViewInd].nOriginY = nOriginY;
  return true;
}

void gslc_ViewSet(gslc_tsGui* pGui,int nViewId)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ViewSet() called with NULL ptr\n");
    return;
  }      
  // Ensure that the view ID is valid
  int nViewInd = GSLC_VIEW_IND_NONE;
  if (nViewId == GSLC_VIEW_ID_SCREEN) {
    nViewInd = GSLC_VIEW_IND_SCREEN;
  } else {
    nViewInd = gslc_ViewFindIndFromId(pGui,nViewId); 
  }

  if (nViewInd == GSLC_VIEW_IND_NONE) {
    fprintf(stderr,"ERROR: ViewSet() invalid ID=%d\n",nViewId);
  }

  // Update the clipping rect temporarily
  if (nViewInd == GSLC_VIEW_IND_SCREEN) {
    // Set to full size of screen
    gslc_DrvSetClipRect(pGui,NULL);
  } else if (gslc_ViewIndValid(pGui,nViewInd)) {
    // Set to user-specified region    
    gslc_DrvSetClipRect(pGui,&(pGui->asView[nViewInd]).rView);
  } else {
    // INVALID
  }
  // Assign the view
  pGui->nViewIndCur = nViewInd;
}


// ------------------------------------------------------------------------
// Tracking Functions
// ------------------------------------------------------------------------

void gslc_CollectTouch(gslc_tsGui* pGui,gslc_tsCollect* pCollect,bool bTouchDown,bool bTouchUp,bool bTouchMove,int nX,int nY)
{
  gslc_tsElem*  pTrackedOld = NULL;
  gslc_tsElem*  pTrackedNew = NULL;
  
  // Fetch the item currently being tracked (if any)
  pTrackedOld = gslc_CollectGetElemTracked(pCollect);
  
  // Reset the in-tracked flag
  bool  bInTracked = false;
  
  if (bTouchDown) {
    // ---------------------------------
    // Touch Down Event
    // ---------------------------------
    
    // End glow on previously tracked element (if any)
    // - We shouldn't really enter a "Touch Down" event
    //   with an element still marked as being tracked
    if (pTrackedOld != NULL) {
      gslc_ElemSetGlow(pTrackedOld,false);
    }
    
    // Determine the new element to start tracking
    pTrackedNew = gslc_CollectFindElemFromCoord(pCollect,nX,nY);
    
    if (pTrackedNew == NULL) {
      // Didn't find an element, so clear the tracking reference   
      gslc_CollectSetElemTracked(pCollect,NULL);
    } else {
      // Found an element, so mark it as being the tracked element

      // Set the new tracked element reference
      gslc_CollectSetElemTracked(pCollect,pTrackedNew);

      // Start glow on new element
      gslc_ElemSetGlow(pTrackedNew,true);
      
      // Notify element for optional custom handling
      // - We do this after we have determined which element should
      //   receive the touch tracking
      gslc_NotifyElemTouch(pGui,pTrackedNew,GSLC_TOUCH_DOWN_IN,nX,nY);
    }
   
  } else if (bTouchUp) {
    // ---------------------------------
    // Touch Up Event
    // ---------------------------------

    if (pTrackedOld != NULL) {
      // Are we still over tracked element?
      bInTracked = gslc_IsInRect(nX,nY,pTrackedOld->rElem);
  
      if (!bInTracked) {
        // Released not over tracked element
        gslc_NotifyElemTouch(pGui,pTrackedOld,GSLC_TOUCH_UP_OUT,nX,nY);
      } else {
        // Notify original tracked element for optional custom handling
        gslc_NotifyElemTouch(pGui,pTrackedOld,GSLC_TOUCH_UP_IN,nX,nY);
      }

      // Clear glow state
      gslc_ElemSetGlow(pTrackedOld,false);      

    }

    // Clear the element tracking state
    gslc_CollectSetElemTracked(pCollect,NULL);
    
  } else if (bTouchMove) {
    // ---------------------------------
    // Touch Move Event
    // ---------------------------------
    
    // Determine if we are still over tracked element
    if (pTrackedOld != NULL) {
      bInTracked = gslc_IsInRect(nX,nY,pTrackedOld->rElem);

      if (!bInTracked) {

        // Not currently over tracked element
        // - Notify tracked element that we moved out of it
        gslc_NotifyElemTouch(pGui,pTrackedOld,GSLC_TOUCH_MOVE_OUT,nX,nY);

        // Ensure the tracked element is no longer glowing
        if (pTrackedOld) {
          gslc_ElemSetGlow(pTrackedOld,false);
        }
      } else {
        // We are still over tracked element
        // - Notify tracked element
        gslc_NotifyElemTouch(pGui,pTrackedOld,GSLC_TOUCH_MOVE_IN,nX,nY);

        // Ensure it is glowing
        gslc_ElemSetGlow(pTrackedOld,true);
      }      
      
    }   

  }  
}

// This routine is responsible for the GUI-level touch event state machine
// and dispatching to the touch even handler for the collection
void gslc_TrackTouch(gslc_tsGui* pGui,gslc_tsCollect* pCollect,int nX,int nY,unsigned nPress)
{
  if ((pGui == NULL) || (pCollect == NULL)) {
    fprintf(stderr,"ERROR: TrackTouch() called with NULL ptr\n");
    return;
  }    

  bool  bTouchDown  = false;
  bool  bTouchUp    = false;
  bool  bTouchMove  = false;
  if ((pGui->nTouchLastPress == 0) && (nPress > 0)) {
    bTouchDown = true;
    #ifdef DBG_TOUCH    
    fprintf(stderr," TS : (%3d,%3d) Pressure=%3u : TouchDown\n",nX,nY,nPress);
    #endif
  } else if ((pGui->nTouchLastPress > 0) && (nPress == 0)) {
    bTouchUp = true;
    #ifdef DBG_TOUCH    
    fprintf(stderr," TS : (%3d,%3d) Pressure=%3u : TouchUp\n",nX,nY,nPress);
    #endif
    
  } else if ((pGui->nTouchLastX != nX) || (pGui->nTouchLastY != nY)) {
    // We only track movement if touch is "down"
    if (nPress > 0) {
      bTouchMove = true;
      #ifdef DBG_TOUCH    
      fprintf(stderr," TS : (%3d,%3d) Pressure=%3u : TouchMove\n",nX,nY,nPress);
      #endif
    }
  }
  
  gslc_CollectTouch(pGui,pCollect,bTouchDown,bTouchUp,bTouchMove,nX,nY);

  // Save raw touch status so that we can detect transitions
  pGui->nTouchLastX      = nX;
  pGui->nTouchLastY      = nY;
  pGui->nTouchLastPress  = nPress;
}



// ------------------------------------------------------------------------
// Touchscreen Functions
// ------------------------------------------------------------------------

#ifdef INC_TS

// POST:
// - pGui->ts mapped to touchscreen device
bool gslc_InitTs(gslc_tsGui* pGui,const char* acDev)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: InitTs() called with NULL ptr\n");
    return false;
  }    
  // TODO: Consider using env "TSLIB_TSDEVICE" instead
  //char* pDevName = NULL;
  //pDevName = getenv("TSLIB_TSDEVICE");
  //pGui->ts = ts_open(pDevName,1);
  
  // Open in non-blocking mode
  pGui->ts = ts_open(acDev,1);
  if (!pGui->ts) {
    fprintf(stderr,"ERROR: TsOpen\n");
    return false;
  }

  if (ts_config(pGui->ts)) {
    fprintf(stderr,"ERROR: TsConfig\n");
    // Clear the tslib pointer so we don't try to call it again
    pGui->ts = NULL;
    return false;
  }
  return true;
}

int gslc_GetTsTouch(gslc_tsGui* pGui,int* pnX,int* pnY,unsigned* pnPress)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: GetTsTouch() called with NULL ptr\n");
    return 0;
  }    
  // In case tslib was not loaded, exit now
  if (pGui->ts == NULL) {
    return 0;
  }
  struct ts_sample   pSamp;
  int nRet    = ts_read(pGui->ts,&pSamp,1);
  (*pnX)      = pSamp.x;
  (*pnY)      = pSamp.y;
  (*pnPress)  = pSamp.pressure;
  return nRet;
}

#endif // INC_TS



// ------------------------------------------------------------------------
// Private Functions
// ------------------------------------------------------------------------


// NOTE: nId is a positive ID specified by the user or
//       GSLC_ID_AUTO if the user wants an auto-generated ID
//       (which will be assigned in Element nId)
// NOTE: When we are creating sub-elements within a compound element,
//       we usually pass nPageId=GSLC_PAGE_NONE. In this mode we
//       won't add the element to any page, but just create the
//       element struct. However, in this mode we can't support
//       auto-generated IDs since we don't know which IDs will
//       be taken when we finally create the compound element.
gslc_tsElem gslc_ElemCreate(gslc_tsGui* pGui,int nElemId,int nPageId,
  gslc_teType nType,gslc_Rect rElem,const char* pStr,int nFontId)
{
  gslc_tsElem sElem;
  // Assign defaults to the element record
  gslc_ResetElem(&sElem);
  
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ElemCreate() called with NULL ptr\n");
    return sElem;
  }  

  gslc_tsPage*    pPage = NULL;
  gslc_tsCollect* pCollect = NULL;
  
  // If we are going to be adding the element to a page then we
  // perform some additional checks
  if (nPageId == GSLC_PAGE_NONE) {
    // This is a temporary element, so we skip the ID collision checks.
    // In this mode we don't support auto ID assignment
    if (nElemId == GSLC_ID_AUTO) {
      fprintf(stderr,"ERROR: ElemCreate() doesn't support temp elements with auto ID\n");
      return sElem;
    }
  } else {
    // Look up the targeted page to ensure that we check its
    // collection for collision with other IDs (or assign the
    // next available if auto-incremented)
    pPage     = gslc_PageFindById(pGui,nPageId);
    if (pPage == NULL) {
      fprintf(stderr,"ERROR: ElemCreate() can't find page (ID=%d)\n",nPageId);
      return sElem;
    }     
    pCollect  = &pPage->sCollect;
  
    // Validate the user-supplied ID
    if (nElemId == GSLC_ID_AUTO) {
      // Get next auto-generated ID
      nElemId = gslc_CollectGetNextId(pCollect);
    } else {
      // Ensure the ID is positive
      if (nElemId < 0) {
        fprintf(stderr,"ERROR: ElemCreate() called with negative ID (%d)\n",nElemId);
        return sElem;
      }
      // Ensure the ID isn't already taken
      if (gslc_CollectFindElemById(pCollect,nElemId) != NULL) {
        fprintf(stderr,"ERROR: ElemCreate() called with existing ID (%d)\n",nElemId);
        return sElem;
      }
    }
  }


  // Override defaults with parameterization
  sElem.nId             = nElemId;
  sElem.rElem           = rElem;
  sElem.nType           = nType;
  gslc_ElemUpdateFont(pGui,&sElem,nFontId);
  if (pStr != NULL) {
    strncpy(sElem.acStr,pStr,GSLC_ELEM_STRLEN_MAX-1);
    sElem.acStr[GSLC_ELEM_STRLEN_MAX-1] = '\0';  // Force termination    
  }  
  
  // If the element creation was successful, then set the valid flag
  sElem.bValid          = true;

  return sElem;
}


// Helper function for gslc_ElemAdd()
// - Note that this copies the content of pElem to collection element array
//   so the pointer can be released after the call
gslc_tsElem* gslc_CollectElemAdd(gslc_tsCollect* pCollect,gslc_tsElem* pElem)
{
  if ((pCollect == NULL) || (pElem == NULL)) {
    fprintf(stderr,"ERROR: CollectElemAdd() called with NULL ptr\n");
    return NULL;
  }    
  
  if (pCollect->nElemCnt+1 >= (pCollect->nElemMax)) {
    fprintf(stderr,"ERROR: CollectElemAdd() too many elements\n");
    return NULL;
  }
  // In case the element creation failed, trap that here
  if (!pElem->bValid) {
    fprintf(stderr,"ERROR: CollectElemAdd() skipping add of invalid element\n");
    return NULL;
  }
  // Add the element to the internal array
  // - This performs a copy so that we can discard the element
  //   pointer after the call is complete
  int nElemInd = pCollect->nElemCnt;  
  pCollect->asElem[nElemInd] = *pElem;
  pCollect->nElemCnt++;

  return &(pCollect->asElem[nElemInd]);    
}


// Add an element to the collection associated with the page
//
// NOTE: The content (not address) of pElem is copied so the pointer
//       can be released after the call.
gslc_tsElem* gslc_ElemAdd(gslc_tsGui* pGui,int nPageId,gslc_tsElem* pElem)
{
  if ((pGui == NULL) || (pElem == NULL)) {
    fprintf(stderr,"ERROR: ElemAdd() called with NULL ptr\n");
    return NULL;
  }    

  // Fetch the page containing the item
  gslc_tsPage*      pPage     = gslc_PageFindById(pGui,nPageId);
  if (pPage == NULL) {
    fprintf(stderr,"ERROR: ElemAdd() page (ID=%d) was not found\n",nPageId);
    return NULL;
  }   
  
  gslc_tsCollect*   pCollect  = &pPage->sCollect;
  
  return gslc_CollectElemAdd(pCollect,pElem);
}


void gslc_ElemSetImage(gslc_tsGui* pGui,gslc_tsElem* pElem,const char* acImage,
  const char* acImageSel)
{
  if ((pGui == NULL) || (pElem == NULL)) {
    fprintf(stderr,"ERROR: ElemSetImage() called with NULL ptr\n");
    return;
  }    

  if (strlen(acImage) > 0) {
    gslc_DrvSetElemImageNorm(pGui,pElem,acImage);
  }

  if (strlen(acImageSel) > 0) {
    gslc_DrvSetElemImageGlow(pGui,pElem,acImageSel);    
  }

}

bool gslc_SetBkgndImage(gslc_tsGui* pGui,char* pStrFname)
{
  if (!gslc_DrvSetBkgndImage(pGui,pStrFname)) {
    return false;
  }
  gslc_PageFlipSet(pGui,true);
  return true;
}

bool gslc_SetBkgndColor(gslc_tsGui* pGui,gslc_Color nCol)
{
  if (!gslc_DrvSetBkgndColor(pGui,nCol)) {
    return false;
  }
  gslc_PageFlipSet(pGui,true);
  return true;  
}



bool gslc_ViewIndValid(gslc_tsGui* pGui,int nViewInd)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ViewIndValid() called with NULL ptr\n");
    return false;
  }    
  if ((nViewInd >= 0) && (nViewInd < (int)(pGui->nViewCnt))) {
    return true;
  } else {
    return false;
  }
}

// Search through the viewport array for a matching ID
int gslc_ViewFindIndFromId(gslc_tsGui* pGui,int nViewId)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ViewFindIndFromId() called with NULL ptr\n");
    return GSLC_IND_NONE;
  }    
  unsigned  nInd;
  int nFound = GSLC_IND_NONE;
  for (nInd=0;nInd<pGui->nViewCnt;nInd++) {
    if (pGui->asView[nInd].nId == nViewId) {
      nFound = nInd;
    }
  }
  return nFound;
}

// Translate a coordinate from local to global according
// to the viewport region and origin.
void gslc_ViewRemapPt(gslc_tsGui* pGui,int16_t* pnX,int16_t* pnY)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ViewRemapPt() called with NULL ptr\n");
    return;
  }    
  int nViewInd = pGui->nViewIndCur;
  if (nViewInd == GSLC_VIEW_IND_SCREEN) {
    return;
  }
  if (!gslc_ViewIndValid(pGui,nViewInd)) {
    return;
  }
  int   nFinalX,nFinalY;
  int   nFrameX,nFrameY;
  int   nOriginX,nOriginY;
  nFrameX   = pGui->asView[nViewInd].rView.x;
  nFrameY   = pGui->asView[nViewInd].rView.y;
  nOriginX  = pGui->asView[nViewInd].nOriginX;
  nOriginY  = pGui->asView[nViewInd].nOriginY;

  nFinalX   = nFrameX + nOriginX + (*pnX);
  nFinalY   = nFrameY + nOriginY + (*pnY);

  *pnX = nFinalX;
  *pnY = nFinalY;
}

// Translate a rectangle's coordinates from local to global according
// to the viewport region and origin.
void gslc_ViewRemapRect(gslc_tsGui* pGui,gslc_Rect* prRect)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: ViewRemapRect() called with NULL ptr\n");
    return;
  }    
  // TODO: Check corrections +/- 1
  int16_t nX0,nY0,nX1,nY1;
  nX0 = prRect->x;
  nY0 = prRect->y;
  nX1 = prRect->x + prRect->w - 1;
  nY1 = prRect->y + prRect->h - 1;
  gslc_ViewRemapPt(pGui,&nX0,&nY0);
  gslc_ViewRemapPt(pGui,&nX1,&nY1);
  prRect->x = nX0;
  prRect->y = nY0;
  prRect->w = nX1-nX0+1;
  prRect->h = nY1-nY0+1;
}


// Notifies an element of a touch event by invoking its touch callback
// function (if enabled) and passing in relative coordinates.
bool gslc_NotifyElemTouch(gslc_tsGui* pGui,gslc_tsElem* pElem,
        gslc_teTouch eTouch,int nX,int nY)
{
  if ((pGui == NULL) || (pElem == NULL)) {
    fprintf(stderr,"ERROR: NotifyElemTouch() called with NULL ptr\n");
    return false;
  }    
  
  // Fetch the extended element callback (if enabled)
  GSLC_CB_TOUCH pfuncXTouch = pElem->pfuncXTouch;
  if (pfuncXTouch != NULL) {
    // Pass in the relative position from corner of element region
    int nElemPosX = pElem->rElem.x;
    int nElemPosY = pElem->rElem.y;
    (*pfuncXTouch)((void*)(pGui),(void*)(pElem),eTouch,nX-nElemPosX,nY-nElemPosY);
  }
  
  return true;
}


// Initialize the element struct to all zeros
void gslc_ResetElem(gslc_tsElem* pElem)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ResetElem() called with NULL ptr\n");
    return;
  }  
  pElem->bValid           = false;
  pElem->nId              = GSLC_ID_NONE;
  pElem->nType            = GSLC_TYPE_BOX;
  pElem->nGroup           = GSLC_GROUP_ID_NONE;
  pElem->rElem            = (gslc_Rect){0,0,0,0};
  pElem->bGlowEn          = false;
  pElem->bGlowing         = false;
  pElem->pvSurfNorm       = NULL;
  pElem->pvSurfGlow       = NULL;
  pElem->bClickEn         = false;
  pElem->bFrameEn         = false;
  pElem->bFillEn          = false;
  pElem->bNeedRedraw      = true;
  pElem->colElemFrame     = GSLC_COL_WHITE;
  pElem->colElemFill      = GSLC_COL_WHITE;
  pElem->colElemFrameGlow = GSLC_COL_WHITE;  
  pElem->colElemFillGlow  = GSLC_COL_WHITE;
  
  pElem->acStr[0]         = '\0';
  pElem->colElemText      = GSLC_COL_WHITE;
  pElem->colElemTextGlow  = GSLC_COL_WHITE;  
  pElem->eTxtAlign        = GSLC_ALIGN_MID_MID;
  pElem->nTxtMargin       = 0;
  pElem->pvTxtFont        = NULL;
  
  pElem->pXData           = NULL;
  pElem->pfuncXDraw       = NULL;
  pElem->pfuncXTouch      = NULL;
  pElem->pfuncXTick       = NULL;
  
  pElem->pElemParent      = NULL;

}

// Initialize the font struct to all zeros
void gslc_ResetFont(gslc_tsFont* pFont)
{
  if (pFont == NULL) {
    fprintf(stderr,"ERROR: ResetFont() called with NULL ptr\n");
    return;
  }    
  pFont->nId = GSLC_FONT_NONE;
  pFont->pvFont = NULL;
}

// Initialize the view struct to all zeros
void gslc_ResetView(gslc_tsView* pView)
{
  if (pView == NULL) {
    fprintf(stderr,"ERROR: ResetView() called with NULL ptr\n");
    return;
  }    
  pView->nId = GSLC_VIEW_ID_NONE;
  pView->rView = (gslc_Rect){0,0,0,0};
  pView->nOriginX = 0;
  pView->nOriginY = 0;
}


// Close down an element
void gslc_ElemDestruct(gslc_tsElem* pElem)
{
  if (pElem == NULL) {
    fprintf(stderr,"ERROR: ElemDestruct() called with NULL ptr\n");
    return;
  }    
  if (pElem->pvSurfNorm != NULL) {
    gslc_DrvSurfaceDestruct(pElem->pvSurfNorm);
    pElem->pvSurfNorm = NULL;
  }
  if (pElem->pvSurfGlow != NULL) {
    gslc_DrvSurfaceDestruct(pElem->pvSurfGlow);
    pElem->pvSurfGlow = NULL;
  }
  
  // TODO: Add callback function so that
  // we can support additional closure actions
  // (eg. closing sub-elements of compound element).
  
}


// Close down a collection
void gslc_CollectDestruct(gslc_tsCollect* pCollect)
{
  if (pCollect == NULL) {
    fprintf(stderr,"ERROR: CollectDestruct() called with NULL ptr\n");
    return;
  }
  unsigned          nElemInd;
  gslc_tsElem*  pElem = NULL;

  for (nElemInd=0;nElemInd<pCollect->nElemCnt;nElemInd++) {
    pElem = &pCollect->asElem[nElemInd];
    gslc_ElemDestruct(pElem);
  }
    
}

// Close down all in page
void gslc_PageDestruct(gslc_tsPage* pPage)
{
  if (pPage == NULL) {
    fprintf(stderr,"ERROR: PageDestruct() called with NULL ptr\n");
    return;
  }      
  gslc_tsCollect* pCollect = &pPage->sCollect;
  gslc_CollectDestruct(pCollect);
}

// Close down all GUI members, including pages and fonts
void gslc_GuiDestruct(gslc_tsGui* pGui)
{
  if (pGui == NULL) {
    fprintf(stderr,"ERROR: GuiDestruct() called with NULL ptr\n");
    return;
  }    
  // Loop through all pages in GUI
  int nPageInd;
  gslc_tsPage*  pPage = NULL;
  for (nPageInd=0;nPageInd<pGui->nPageCnt;nPageInd++) {
    pPage = &pGui->asPage[nPageInd];
    gslc_PageDestruct(pPage);
  }
  
  // TODO: Consider moving into main element array
  if (pGui->pvSurfBkgnd != NULL) {
    gslc_DrvSurfaceDestruct(pGui->pvSurfBkgnd);
    pGui->pvSurfBkgnd = NULL;
  }
  
  // Close all fonts
  gslc_DrvFontsDestruct(pGui);

}


// ================================
// Private: Element Collection

void gslc_CollectReset(gslc_tsCollect* pCollect,gslc_tsElem* asElem,unsigned nElemMax)
{
  if (pCollect == NULL) {
    fprintf(stderr,"ERROR: CollectReset() called with NULL ptr\n");
    return;
  }  
  pCollect->pElemParent       = NULL;
  
  pCollect->nElemMax          = nElemMax;
  pCollect->nElemCnt          = GSLC_IND_FIRST;
  
  pCollect->nElemAutoIdNext   = GSLC_ID_AUTO_BASE;
  
  pCollect->pElemTracked      = NULL;
  
  // Save the pointer to the element array
  pCollect->asElem = asElem;
  
  int nInd;
  for (nInd=0;nInd<nElemMax;nInd++) {
    gslc_ResetElem(&(pCollect->asElem[nInd]));
  }
}


gslc_tsElem* gslc_CollectFindElemById(gslc_tsCollect* pCollect,int nElemId)
{
  if (pCollect == NULL) {
    fprintf(stderr,"ERROR: CollectFindElemById() called with NULL ptr\n");
    return NULL;
  }  
  gslc_tsElem*  pFoundElem = NULL;
  unsigned          nInd;
  if (nElemId == GSLC_ID_TEMP) {
    // ERROR: Don't expect to do this
    fprintf(stderr,"ERROR: CollectFindElemById() searching for temp ID\n");    
    return NULL;
  }
  for (nInd=GSLC_IND_FIRST;nInd<pCollect->nElemCnt;nInd++) {
    if (pCollect->asElem[nInd].nId == nElemId) {
      pFoundElem = &(pCollect->asElem[nInd]);
      break;
    }
  }
  return pFoundElem;
}

int gslc_CollectGetNextId(gslc_tsCollect* pCollect)
{
  int nElemId = pCollect->nElemAutoIdNext;
  pCollect->nElemAutoIdNext++;
  return nElemId;
}

gslc_tsElem* gslc_CollectGetElemTracked(gslc_tsCollect* pCollect)
{
  return pCollect->pElemTracked;
}

void gslc_CollectSetElemTracked(gslc_tsCollect* pCollect,gslc_tsElem* pElem)
{
  pCollect->pElemTracked = pElem;
}


// Redraw the element collection
// - Only the elements that have been marked as needing redraw
//   are rendered.
void gslc_CollectRedraw(gslc_tsGui* pGui,gslc_tsCollect* pCollect,bool bRedrawAll)
{
  unsigned          nInd;
  gslc_tsElem*  pElem = NULL;
  // Draw elements (as needed)
  for (nInd=GSLC_IND_FIRST;nInd<pCollect->nElemCnt;nInd++) {
    pElem = &(pCollect->asElem[nInd]);       
    if ((pElem->bNeedRedraw) || (bRedrawAll)) {      
      // Draw the element
      // - Note that this also clears the redraw flag
      gslc_ElemDrawByRef(pGui,pElem);        
    }
  }
}

void gslc_CollectTick(gslc_tsGui* pGui,gslc_tsCollect* pCollect)
{
  unsigned          nInd;
  gslc_tsElem*  pElem = NULL;
  // Loop through elements in collection and issue tick callback
  for (nInd=GSLC_IND_FIRST;nInd<pCollect->nElemCnt;nInd++) {
    pElem = &(pCollect->asElem[nInd]);
    if (pElem->pfuncXTick != NULL) {
      (*pElem->pfuncXTick)((void*)(pGui),(void*)(pElem));
    }      
  }  
}


// Find an element index in a collection from a coordinate
gslc_tsElem* gslc_CollectFindElemFromCoord(gslc_tsCollect* pCollect,int nX, int nY)
{
  unsigned  nInd;
  bool      bFound = false;
  gslc_tsElem*  pElem = NULL;
  gslc_tsElem*  pFoundElem = NULL;

  #ifdef DBG_TOUCH
  // Highlight current touch for coordinate debug
  gslc_Rect    rMark = gslc_ExpandRect((gslc_Rect){(int16_t)nX,(int16_t)nY,1,1},1,1);
  gslc_DrawFrameRect(pGui,rMark,GSLC_COL_YELLOW);
  printf("    CollectFindElemFromCoord(%3d,%3d):\n",nX,nY);
  #endif

  for (nInd=GSLC_IND_FIRST;nInd<pCollect->nElemCnt;nInd++) {

    pElem = &pCollect->asElem[nInd];
    
    // Are we within the rect?
    if (gslc_IsInRect(nX,nY,pElem->rElem)) {
      #ifdef DBG_TOUCH
      printf("      [%3u]:In  ",nInd);
      #endif

      // Only return element index if clickable
      if (pElem->bClickEn) {
          if (bFound) {
              fprintf(stderr,"WARNING: Overlapping clickable regions detected\n");
          }
          bFound = true;
          // NOTE: If multiple hits were found, the last one in the
          // collection will be returned
          pFoundElem = pElem;
          #ifdef DBG_TOUCH
          printf("En \n");
          #endif
          // No more search to do
          break;
      } else {
          #ifdef DBG_TOUCH
          printf("Dis\n");
          #endif
      }

    } // gslc_IsInRect()
    else {
      #ifdef DBG_TOUCH
      printf("      [%3u]:Out\n",nInd);
      #endif
    }
  }

  // Return pointer or NULL if none found
  return pFoundElem;
}

// Go through all elements in a collection and set the parent
// element pointer.
void gslc_CollectSetParent(gslc_tsCollect* pCollect,gslc_tsElem* pElemParent)
{
  gslc_tsElem*  pElem = NULL;
  int               nInd;
  for (nInd=GSLC_IND_FIRST;nInd<pCollect->nElemCnt;nInd++) {
    pElem = &pCollect->asElem[nInd];
    pElem->pElemParent = pElemParent;
  }
}