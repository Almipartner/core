/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <vcl/event.hxx>
#include <vcl/svapp.hxx>
#include <vcl/wizdlg.hxx>
#include <tools/debug.hxx>
#include <tools/diagnose_ex.h>
#include <strings.hrc>
#include <svdata.hxx>
#include <stack>
#include "wizimpldata.hxx"

#define HID_WIZARD_NEXT                                        "SVT_HID_WIZARD_NEXT"
#define HID_WIZARD_PREVIOUS                                    "SVT_HID_WIZARD_PREVIOUS"

#define WIZARDDIALOG_BUTTON_OFFSET_Y        6
#define WIZARDDIALOG_BUTTON_DLGOFFSET_X     6
#define WIZARDDIALOG_VIEW_DLGOFFSET_X       6
#define WIZARDDIALOG_VIEW_DLGOFFSET_Y       6

namespace vcl
{
    //= WizardPageImplData
    OWizardPage::OWizardPage(TabPageParent pParent, const OUString& rUIXMLDescription, const OString& rID)
        : TabPage(pParent.pPage ? Application::GetDefDialogParent() : pParent.pParent.get()) //just drag this along hidden in this scenario
        , m_xBuilder(pParent.pPage ? Application::CreateBuilder(pParent.pPage, rUIXMLDescription)
                                   : Application::CreateInterimBuilder(this, rUIXMLDescription))
        , m_xContainer(m_xBuilder->weld_container(rID))
    {
    }

    OWizardPage::~OWizardPage()
    {
        disposeOnce();
    }

    void OWizardPage::dispose()
    {
        m_xBuilder.reset();
        TabPage::dispose();
    }

    void OWizardPage::initializePage()
    {
    }

    void OWizardPage::ActivatePage()
    {
        TabPage::ActivatePage();
        updateDialogTravelUI();
    }

    void OWizardPage::updateDialogTravelUI()
    {
        RoadmapWizard* pWizardMachine = dynamic_cast<RoadmapWizard*>(GetParent());
        if ( pWizardMachine )
            pWizardMachine->updateTravelUI();
    }

    bool OWizardPage::canAdvance() const
    {
        return true;
    }

    bool OWizardPage::commitPage( WizardTypes::CommitPageReason )
    {
        return true;
    }

    void RoadmapWizard::ImplInitData()
    {
        mpFirstPage     = nullptr;
        mpFirstBtn      = nullptr;
        mpCurTabPage    = nullptr;
        mpPrevBtn       = nullptr;
        mpNextBtn       = nullptr;
        mpViewWindow    = nullptr;
        mnCurLevel      = 0;
        meViewAlign     = WindowAlign::Left;
        mbEmptyViewMargin =  false;
        mnLeftAlignCount = 0;

        maWizardLayoutIdle.SetPriority(TaskPriority::RESIZE);
        maWizardLayoutIdle.SetInvokeHandler( LINK( this, RoadmapWizard, ImplHandleWizardLayoutTimerHdl ) );
    }

    void RoadmapWizard::SetLeftAlignedButtonCount( sal_Int16 _nCount )
    {
        mnLeftAlignCount = _nCount;
    }

    void RoadmapWizard::SetEmptyViewMargin()
    {
        mbEmptyViewMargin = true;
    }

    void RoadmapWizard::ImplCalcSize( Size& rSize )
    {
        // calculate ButtonBar height
        long                nMaxHeight = 0;
        ImplWizButtonData*  pBtnData = mpFirstBtn;
        while ( pBtnData )
        {
            long nBtnHeight = pBtnData->mpButton->GetSizePixel().Height();
            if ( nBtnHeight > nMaxHeight )
                nMaxHeight = nBtnHeight;
            pBtnData = pBtnData->mpNext;
        }
        if ( nMaxHeight )
            nMaxHeight += WIZARDDIALOG_BUTTON_OFFSET_Y*2;
        rSize.AdjustHeight(nMaxHeight );

        // add in the view window size
        if ( mpViewWindow && mpViewWindow->IsVisible() )
        {
            Size aViewSize = mpViewWindow->GetSizePixel();
            if ( meViewAlign == WindowAlign::Top )
                rSize.AdjustHeight(aViewSize.Height() );
            else if ( meViewAlign == WindowAlign::Left )
                rSize.AdjustWidth(aViewSize.Width() );
            else if ( meViewAlign == WindowAlign::Bottom )
                rSize.AdjustHeight(aViewSize.Height() );
            else if ( meViewAlign == WindowAlign::Right )
                rSize.AdjustWidth(aViewSize.Width() );
        }
    }

    void RoadmapWizard::queue_resize(StateChangedType /*eReason*/)
    {
        if (maWizardLayoutIdle.IsActive())
            return;
        if (IsInClose())
            return;
        maWizardLayoutIdle.Start();
    }

    IMPL_LINK_NOARG(RoadmapWizard, ImplHandleWizardLayoutTimerHdl, Timer*, void)
    {
        ImplPosCtrls();
        ImplPosTabPage();
    }

    void RoadmapWizard::ImplPosCtrls()
    {
        Size    aDlgSize = GetOutputSizePixel();
        long    nBtnWidth = 0;
        long    nMaxHeight = 0;
        long    nOffY = aDlgSize.Height();

        ImplWizButtonData* pBtnData = mpFirstBtn;
        int j = 0;
        while ( pBtnData )
        {
            if (j >= mnLeftAlignCount)
            {
                Size aBtnSize = pBtnData->mpButton->GetSizePixel();
                long nBtnHeight = aBtnSize.Height();
                if ( nBtnHeight > nMaxHeight )
                    nMaxHeight = nBtnHeight;
                nBtnWidth += aBtnSize.Width();
                nBtnWidth += pBtnData->mnOffset;
            }
            pBtnData = pBtnData->mpNext;
            j++;
        }

        if ( nMaxHeight )
        {
            long nOffX = aDlgSize.Width()-nBtnWidth-WIZARDDIALOG_BUTTON_DLGOFFSET_X;
            long nOffLeftAlignX = LogicalCoordinateToPixel(6);
            nOffY -= WIZARDDIALOG_BUTTON_OFFSET_Y+nMaxHeight;

            pBtnData = mpFirstBtn;
            int i = 0;
            while ( pBtnData )
            {
                Size aBtnSize = pBtnData->mpButton->GetSizePixel();
                if (i >= mnLeftAlignCount)
                {
                    Point aPos( nOffX, nOffY+((nMaxHeight-aBtnSize.Height())/2) );
                    pBtnData->mpButton->SetPosPixel( aPos );
                    nOffX += aBtnSize.Width();
                    nOffX += pBtnData->mnOffset;
                }
                else
                {
                    Point aPos( nOffLeftAlignX, nOffY+((nMaxHeight-aBtnSize.Height())/2) );
                    pBtnData->mpButton->SetPosPixel( aPos );
                    nOffLeftAlignX += aBtnSize.Width();
                    nOffLeftAlignX += pBtnData->mnOffset;
                }

                pBtnData = pBtnData->mpNext;
                i++;
            }

            nOffY -= WIZARDDIALOG_BUTTON_OFFSET_Y;
        }

        if ( !(mpViewWindow && mpViewWindow->IsVisible()) )
            return;

        long    nViewOffX = 0;
        long    nViewOffY = 0;
        long    nViewWidth = 0;
        long    nViewHeight = 0;
        long    nDlgHeight = nOffY;
        PosSizeFlags nViewPosFlags = PosSizeFlags::Pos;
        if ( meViewAlign == WindowAlign::Top )
        {
            nViewOffX       = WIZARDDIALOG_VIEW_DLGOFFSET_X;
            nViewOffY       = WIZARDDIALOG_VIEW_DLGOFFSET_Y;
            nViewWidth      = aDlgSize.Width()-(WIZARDDIALOG_VIEW_DLGOFFSET_X*2);
            nViewPosFlags  |= PosSizeFlags::Width;
        }
        else if ( meViewAlign == WindowAlign::Left )
        {
            if ( mbEmptyViewMargin )
            {
                nViewOffX       = 0;
                nViewOffY       = 0;
                nViewHeight     = nDlgHeight;
            }
            else
            {
                nViewOffX       = WIZARDDIALOG_VIEW_DLGOFFSET_X;
                nViewOffY       = WIZARDDIALOG_VIEW_DLGOFFSET_Y;
                nViewHeight     = nDlgHeight-(WIZARDDIALOG_VIEW_DLGOFFSET_Y*2);
            }
            nViewPosFlags  |= PosSizeFlags::Height;
        }
        else if ( meViewAlign == WindowAlign::Bottom )
        {
            nViewOffX       = WIZARDDIALOG_VIEW_DLGOFFSET_X;
            nViewOffY       = nDlgHeight-mpViewWindow->GetSizePixel().Height()-WIZARDDIALOG_VIEW_DLGOFFSET_Y;
            nViewWidth      = aDlgSize.Width()-(WIZARDDIALOG_VIEW_DLGOFFSET_X*2);
            nViewPosFlags  |= PosSizeFlags::Width;
        }
        else if ( meViewAlign == WindowAlign::Right )
        {
            nViewOffX       = aDlgSize.Width()-mpViewWindow->GetSizePixel().Width()-WIZARDDIALOG_VIEW_DLGOFFSET_X;
            nViewOffY       = WIZARDDIALOG_VIEW_DLGOFFSET_Y;
            nViewHeight     = nDlgHeight-(WIZARDDIALOG_VIEW_DLGOFFSET_Y*2);
            nViewPosFlags  |= PosSizeFlags::Height;
        }
        mpViewWindow->setPosSizePixel( nViewOffX, nViewOffY,
                                       nViewWidth, nViewHeight,
                                       nViewPosFlags );
    }

    long RoadmapWizard::LogicalCoordinateToPixel(int iCoordinate){
        Size aLocSize = LogicToPixel(Size(iCoordinate, 0), MapMode(MapUnit::MapAppFont));
        int iPixelCoordinate =  aLocSize.Width();
        return iPixelCoordinate;
    }

    void RoadmapWizard::ImplPosTabPage()
    {
        if ( !mpCurTabPage )
            return;

        if ( !IsInInitShow() )
        {
            // #100199# - On Unix initial size is equal to screen size, on Windows
            // it's 0,0. One cannot calculate the size unless dialog is visible.
            if ( !IsReallyVisible() )
                return;
        }

        // calculate height of ButtonBar
        long                nMaxHeight = 0;
        ImplWizButtonData*  pBtnData = mpFirstBtn;
        while ( pBtnData )
        {
            long nBtnHeight = pBtnData->mpButton->GetSizePixel().Height();
            if ( nBtnHeight > nMaxHeight )
                nMaxHeight = nBtnHeight;
            pBtnData = pBtnData->mpNext;
        }
        if ( nMaxHeight )
            nMaxHeight += WIZARDDIALOG_BUTTON_OFFSET_Y*2;

        // position TabPage
        Size aDlgSize = GetOutputSizePixel();
        aDlgSize.AdjustHeight( -nMaxHeight );
        long nOffX = 0;
        long nOffY = 0;
        if ( mpViewWindow && mpViewWindow->IsVisible() )
        {
            Size aViewSize = mpViewWindow->GetSizePixel();
            if ( meViewAlign == WindowAlign::Top )
            {
                nOffY += aViewSize.Height()+WIZARDDIALOG_VIEW_DLGOFFSET_Y;
                aDlgSize.AdjustHeight( -(aViewSize.Height()+WIZARDDIALOG_VIEW_DLGOFFSET_Y) );
            }
            else if ( meViewAlign == WindowAlign::Left )
            {
                long nViewOffset = mbEmptyViewMargin ? 0 : WIZARDDIALOG_VIEW_DLGOFFSET_X;
                nOffX += aViewSize.Width() + nViewOffset;
                aDlgSize.AdjustWidth( -nOffX );
            }
            else if ( meViewAlign == WindowAlign::Bottom )
                aDlgSize.AdjustHeight( -(aViewSize.Height()+WIZARDDIALOG_VIEW_DLGOFFSET_Y) );
            else if ( meViewAlign == WindowAlign::Right )
                aDlgSize.AdjustWidth( -(aViewSize.Width()+WIZARDDIALOG_VIEW_DLGOFFSET_X) );
        }
        Point aPos( nOffX, nOffY );
        mpCurTabPage->SetPosSizePixel( aPos, aDlgSize );
    }

    void RoadmapWizard::ImplShowTabPage( TabPage* pTabPage )
    {
        if ( mpCurTabPage == pTabPage )
            return;

        TabPage* pOldTabPage = mpCurTabPage;
        if ( pOldTabPage )
            pOldTabPage->DeactivatePage();

        mpCurTabPage = pTabPage;
        if ( pTabPage )
        {
            ImplPosTabPage();
            pTabPage->ActivatePage();
            pTabPage->Show();
        }

        if ( pOldTabPage )
            pOldTabPage->Hide();
    }

    TabPage* RoadmapWizard::ImplGetPage( sal_uInt16 nLevel ) const
    {
        sal_uInt16              nTempLevel = 0;
        ImplWizPageData*    pPageData = mpFirstPage;
        while ( pPageData )
        {
            if ( (nTempLevel == nLevel) || !pPageData->mpNext )
                break;

            nTempLevel++;
            pPageData = pPageData->mpNext;
        }

        if ( pPageData )
            return pPageData->mpPage;
        return nullptr;
    }

    void RoadmapWizard::implConstruct( const WizardButtonFlags _nButtonFlags )
    {
        m_xWizardImpl->sTitleBase = GetText();

        // create the buttons according to the wizard button flags
        // the help button
        if (_nButtonFlags & WizardButtonFlags::HELP)
        {
            m_pHelp= VclPtr<HelpButton>::Create(this, WB_TABSTOP);
            m_pHelp->SetSizePixel(LogicToPixel(Size(50, 14), MapMode(MapUnit::MapAppFont)));
            m_pHelp->Show();
            AddButton( m_pHelp, WIZARDDIALOG_BUTTON_STDOFFSET_X);
        }

        // the previous button
        if (_nButtonFlags & WizardButtonFlags::PREVIOUS)
        {
            m_pPrevPage = VclPtr<PushButton>::Create(this, WB_TABSTOP);
            m_pPrevPage->SetHelpId( HID_WIZARD_PREVIOUS );
            m_pPrevPage->SetSizePixel(LogicToPixel(Size(50, 14), MapMode(MapUnit::MapAppFont)));
            m_pPrevPage->SetText(VclResId(STR_WIZDLG_PREVIOUS));
            m_pPrevPage->Show();
            m_pPrevPage->set_id("previous");

            if (_nButtonFlags & WizardButtonFlags::NEXT)
                AddButton( m_pPrevPage, ( WIZARDDIALOG_BUTTON_SMALLSTDOFFSET_X) );      // half x-offset to the next button
            else
                AddButton( m_pPrevPage, WIZARDDIALOG_BUTTON_STDOFFSET_X );
            SetPrevButton( m_pPrevPage );
            m_pPrevPage->SetClickHdl( LINK( this, RoadmapWizard, OnPrevPage ) );
        }

        // the next button
        if (_nButtonFlags & WizardButtonFlags::NEXT)
        {
            m_pNextPage = VclPtr<PushButton>::Create(this, WB_TABSTOP);
            m_pNextPage->SetHelpId( HID_WIZARD_NEXT );
            m_pNextPage->SetSizePixel(LogicToPixel(Size(50, 14), MapMode(MapUnit::MapAppFont)));
            m_pNextPage->SetText(VclResId(STR_WIZDLG_NEXT));
            m_pNextPage->Show();
            m_pNextPage->set_id("next");

            AddButton( m_pNextPage, WIZARDDIALOG_BUTTON_STDOFFSET_X );
            SetNextButton( m_pNextPage );
            m_pNextPage->SetClickHdl( LINK( this, RoadmapWizard, OnNextPage ) );
        }

        // the finish button
        if (_nButtonFlags & WizardButtonFlags::FINISH)
        {
            m_pFinish = VclPtr<OKButton>::Create(this, WB_TABSTOP);
            m_pFinish->SetSizePixel(LogicToPixel(Size(50, 14), MapMode(MapUnit::MapAppFont)));
            m_pFinish->SetText(VclResId(STR_WIZDLG_FINISH));
            m_pFinish->Show();
            m_pFinish->set_id("finish");

            AddButton( m_pFinish, WIZARDDIALOG_BUTTON_STDOFFSET_X );
            m_pFinish->SetClickHdl( LINK( this, RoadmapWizard, OnFinish ) );
        }

        // the cancel button
        if (_nButtonFlags & WizardButtonFlags::CANCEL)
        {
            m_pCancel = VclPtr<CancelButton>::Create(this, WB_TABSTOP);
            m_pCancel->SetSizePixel(LogicToPixel(Size(50, 14), MapMode(MapUnit::MapAppFont)));
            m_pCancel->Show();

            AddButton( m_pCancel, WIZARDDIALOG_BUTTON_STDOFFSET_X );
        }
    }

    void RoadmapWizard::Resize()
    {
        if ( IsReallyShown() && !IsInInitShow() )
        {
            ImplPosCtrls();
            ImplPosTabPage();
        }

        Dialog::Resize();
    }

    void RoadmapWizard::implUpdateTitle()
    {
        OUString sCompleteTitle(m_xWizardImpl->sTitleBase);

        // append the page title
        TabPage* pCurrentPage = GetPage(getCurrentState());
        if ( pCurrentPage && !pCurrentPage->GetText().isEmpty() )
        {
            sCompleteTitle += " - " + pCurrentPage->GetText();
        }

        SetText(sCompleteTitle);
    }

    void RoadmapWizard::CalcAndSetSize()
    {
        Size aDlgSize = GetPageSizePixel();
        if ( !aDlgSize.Width() || !aDlgSize.Height() )
        {
            ImplWizPageData*  pPageData = mpFirstPage;
            while ( pPageData )
            {
                if ( pPageData->mpPage )
                {
                    Size aPageSize = pPageData->mpPage->GetSizePixel();
                    if ( aPageSize.Width() > aDlgSize.Width() )
                        aDlgSize.setWidth( aPageSize.Width() );
                    if ( aPageSize.Height() > aDlgSize.Height() )
                        aDlgSize.setHeight( aPageSize.Height() );
                }

                pPageData = pPageData->mpNext;
            }
        }
        ImplCalcSize( aDlgSize );
        SetOutputSizePixel( aDlgSize );
    }

    void RoadmapWizard::StateChanged( StateChangedType nType )
    {
        if ( nType == StateChangedType::InitShow )
        {
            if ( IsDefaultSize() )
            {
                CalcAndSetSize();
            }

            ImplPosCtrls();
            ImplPosTabPage();
            ImplShowTabPage( ImplGetPage( mnCurLevel ) );
        }

        Dialog::StateChanged( nType );
    }

    bool RoadmapWizard::EventNotify( NotifyEvent& rNEvt )
    {
        if ( (rNEvt.GetType() == MouseNotifyEvent::KEYINPUT) && mpPrevBtn && mpNextBtn )
        {
            const KeyEvent* pKEvt = rNEvt.GetKeyEvent();
            vcl::KeyCode aKeyCode = pKEvt->GetKeyCode();
            sal_uInt16 nKeyCode = aKeyCode.GetCode();

            if ( aKeyCode.IsMod1() )
            {
                if ( aKeyCode.IsShift() || (nKeyCode == KEY_PAGEUP) )
                {
                    if ( (nKeyCode == KEY_TAB) || (nKeyCode == KEY_PAGEUP) )
                    {
                        if ( mpPrevBtn->IsVisible() &&
                             mpPrevBtn->IsEnabled() && mpPrevBtn->IsInputEnabled() )
                        {
                            mpPrevBtn->SetPressed( true );
                            mpPrevBtn->SetPressed( false );
                            mpPrevBtn->Click();
                        }
                        return true;
                    }
                }
                else
                {
                    if ( (nKeyCode == KEY_TAB) || (nKeyCode == KEY_PAGEDOWN) )
                    {
                        if ( mpNextBtn->IsVisible() &&
                             mpNextBtn->IsEnabled() && mpNextBtn->IsInputEnabled() )
                        {
                            mpNextBtn->SetPressed( true );
                            mpNextBtn->SetPressed( false );
                            mpNextBtn->Click();
                        }
                        return true;
                    }
                }
            }
        }

        return Dialog::EventNotify( rNEvt );
    }

    void RoadmapWizard::setTitleBase(const OUString& _rTitleBase)
    {
        m_xWizardImpl->sTitleBase = _rTitleBase;
        implUpdateTitle();
    }

    TabPage* RoadmapWizard::GetOrCreatePage( const WizardTypes::WizardState i_nState )
    {
        if ( nullptr == GetPage( i_nState ) )
        {
            VclPtr<TabPage> pNewPage = createPage( i_nState );
            DBG_ASSERT( pNewPage, "RoadmapWizard::GetOrCreatePage: invalid new page (NULL)!" );

            // fill up the page sequence of our base class (with dummies)
            while ( m_xWizardImpl->nFirstUnknownPage < i_nState )
            {
                AddPage( nullptr );
                ++m_xWizardImpl->nFirstUnknownPage;
            }

            if ( m_xWizardImpl->nFirstUnknownPage == i_nState )
            {
                // encountered this page number the first time
                AddPage( pNewPage );
                ++m_xWizardImpl->nFirstUnknownPage;
            }
            else
                // already had this page - just change it
                SetPage( i_nState, pNewPage );
        }
        return GetPage( i_nState );
    }

    void RoadmapWizard::ActivatePage()
    {
        maActivateHdl.Call( this );

        WizardTypes::WizardState nCurrentLevel = GetCurLevel();
        GetOrCreatePage( nCurrentLevel );

        enterState( nCurrentLevel );
    }

    bool RoadmapWizard::DeactivatePage()
    {
        WizardTypes::WizardState nCurrentState = getCurrentState();
        return leaveState(nCurrentState);
    }

    void RoadmapWizard::defaultButton(WizardButtonFlags _nWizardButtonFlags)
    {
        // the new default button
        PushButton* pNewDefButton = nullptr;
        if (m_pFinish && (_nWizardButtonFlags & WizardButtonFlags::FINISH))
            pNewDefButton = m_pFinish;
        if (m_pNextPage && (_nWizardButtonFlags & WizardButtonFlags::NEXT))
            pNewDefButton = m_pNextPage;
        if (m_pPrevPage && (_nWizardButtonFlags & WizardButtonFlags::PREVIOUS))
            pNewDefButton = m_pPrevPage;
        if (m_pHelp && (_nWizardButtonFlags & WizardButtonFlags::HELP))
            pNewDefButton = m_pHelp;
        if (m_pCancel && (_nWizardButtonFlags & WizardButtonFlags::CANCEL))
            pNewDefButton = m_pCancel;

        if ( pNewDefButton )
            defaultButton( pNewDefButton );
        else
            implResetDefault( this );
    }

    bool RoadmapWizard::ShowNextPage()
    {
        return ShowPage( mnCurLevel+1 );
    }

    bool RoadmapWizard::ShowPrevPage()
    {
        if ( !mnCurLevel )
            return false;
        return ShowPage( mnCurLevel-1 );
    }

    bool RoadmapWizard::ShowPage( sal_uInt16 nLevel )
    {
        if ( DeactivatePage() )
        {
            mnCurLevel = nLevel;
            ActivatePage();
            ImplShowTabPage( ImplGetPage( mnCurLevel ) );
            return true;
        }
        else
            return false;
    }

    bool RoadmapWizard::Finish( long nResult )
    {
        if ( DeactivatePage() )
        {
            if ( mpCurTabPage )
                mpCurTabPage->DeactivatePage();

            if ( IsInExecute() )
                EndDialog( nResult );
            else if ( GetStyle() & WB_CLOSEABLE )
                Close();
            return true;
        }
        else
            return false;
    }

    void RoadmapWizard::AddPage( TabPage* pPage )
    {
        ImplWizPageData* pNewPageData = new ImplWizPageData;
        pNewPageData->mpNext    = nullptr;
        pNewPageData->mpPage    = pPage;

        if ( !mpFirstPage )
            mpFirstPage = pNewPageData;
        else
        {
            ImplWizPageData* pPageData = mpFirstPage;
            while ( pPageData->mpNext )
                pPageData = pPageData->mpNext;
            pPageData->mpNext = pNewPageData;
        }
    }

    void RoadmapWizard::RemovePage( TabPage* pPage )
    {
        ImplWizPageData*  pPrevPageData = nullptr;
        ImplWizPageData*  pPageData = mpFirstPage;
        while ( pPageData )
        {
            if ( pPageData->mpPage == pPage )
            {
                if ( pPrevPageData )
                    pPrevPageData->mpNext = pPageData->mpNext;
                else
                    mpFirstPage = pPageData->mpNext;
                if ( pPage == mpCurTabPage )
                    mpCurTabPage = nullptr;
                delete pPageData;
                return;
            }

            pPrevPageData = pPageData;
            pPageData = pPageData->mpNext;
        }

        OSL_FAIL( "RoadmapWizard::RemovePage() - Page not in list" );
    }

    void RoadmapWizard::SetPage( sal_uInt16 nLevel, TabPage* pPage )
    {
        sal_uInt16              nTempLevel = 0;
        ImplWizPageData*    pPageData = mpFirstPage;
        while ( pPageData )
        {
            if ( (nTempLevel == nLevel) || !pPageData->mpNext )
                break;

            nTempLevel++;
            pPageData = pPageData->mpNext;
        }

        if ( pPageData )
        {
            if ( pPageData->mpPage == mpCurTabPage )
                mpCurTabPage = nullptr;
            pPageData->mpPage = pPage;
        }
    }

    TabPage* RoadmapWizard::GetPage( sal_uInt16 nLevel ) const
    {
        sal_uInt16 nTempLevel = 0;

        for (ImplWizPageData* pPageData = mpFirstPage; pPageData;
             pPageData = pPageData->mpNext)
        {
            if ( nTempLevel == nLevel )
                return pPageData->mpPage;
            nTempLevel++;
        }

        return nullptr;
    }

    void RoadmapWizard::AddButton( Button* pButton, long nOffset )
    {
        ImplWizButtonData* pNewBtnData = new ImplWizButtonData;
        pNewBtnData->mpNext     = nullptr;
        pNewBtnData->mpButton   = pButton;
        pNewBtnData->mnOffset   = nOffset;

        if ( !mpFirstBtn )
            mpFirstBtn = pNewBtnData;
        else
        {
            ImplWizButtonData* pBtnData = mpFirstBtn;
            while ( pBtnData->mpNext )
                pBtnData = pBtnData->mpNext;
            pBtnData->mpNext = pNewBtnData;
        }
    }

    void RoadmapWizard::RemoveButton( Button* pButton )
    {
        ImplWizButtonData*  pPrevBtnData = nullptr;
        ImplWizButtonData*  pBtnData = mpFirstBtn;
        while ( pBtnData )
        {
            if ( pBtnData->mpButton == pButton )
            {
                if ( pPrevBtnData )
                    pPrevBtnData->mpNext = pBtnData->mpNext;
                else
                    mpFirstBtn = pBtnData->mpNext;
                delete pBtnData;
                return;
            }

            pPrevBtnData = pBtnData;
            pBtnData = pBtnData->mpNext;
        }

        OSL_FAIL( "RoadmapWizard::RemoveButton() - Button not in list" );
    }

    void RoadmapWizard::implResetDefault(vcl::Window const * _pWindow)
    {
        vcl::Window* pChildLoop = _pWindow->GetWindow(GetWindowType::FirstChild);
        while (pChildLoop)
        {
            // does the window participate in the tabbing order?
            if (pChildLoop->GetStyle() & WB_DIALOGCONTROL)
                implResetDefault(pChildLoop);

            // is it a button?
            WindowType eType = pChildLoop->GetType();
            if  (   (WindowType::PUSHBUTTON == eType)
                ||  (WindowType::OKBUTTON == eType)
                ||  (WindowType::CANCELBUTTON == eType)
                ||  (WindowType::HELPBUTTON == eType)
                ||  (WindowType::IMAGEBUTTON == eType)
                ||  (WindowType::MENUBUTTON == eType)
                ||  (WindowType::MOREBUTTON == eType)
                )
            {
                pChildLoop->SetStyle(pChildLoop->GetStyle() & ~WB_DEFBUTTON);
            }

            // the next one ...
            pChildLoop = pChildLoop->GetWindow(GetWindowType::Next);
        }
    }

    void RoadmapWizard::defaultButton(PushButton* _pNewDefButton)
    {
        // loop through all (direct and indirect) descendants which participate in our tabbing order, and
        // reset the WB_DEFBUTTON for every window which is a button
        implResetDefault(this);

        // set its new style
        if (_pNewDefButton)
            _pNewDefButton->SetStyle(_pNewDefButton->GetStyle() | WB_DEFBUTTON);
    }

    void RoadmapWizard::enableButtons(WizardButtonFlags _nWizardButtonFlags, bool _bEnable)
    {
        if (m_pFinish && (_nWizardButtonFlags & WizardButtonFlags::FINISH))
            m_pFinish->Enable(_bEnable);
        if (m_pNextPage && (_nWizardButtonFlags & WizardButtonFlags::NEXT))
            m_pNextPage->Enable(_bEnable);
        if (m_pPrevPage && (_nWizardButtonFlags & WizardButtonFlags::PREVIOUS))
            m_pPrevPage->Enable(_bEnable);
        if (m_pHelp && (_nWizardButtonFlags & WizardButtonFlags::HELP))
            m_pHelp->Enable(_bEnable);
        if (m_pCancel && (_nWizardButtonFlags & WizardButtonFlags::CANCEL))
            m_pCancel->Enable(_bEnable);
    }

    bool RoadmapWizard::leaveState(WizardTypes::WizardState)
    {
        // no need to ask the page here.
        // If we reach this point, we already gave the current page the chance to commit it's data,
        // and it was allowed to commit it's data

        return true;
    }

    bool RoadmapWizard::onFinish()
    {
        return Finish( RET_OK );
    }

    IMPL_LINK_NOARG(RoadmapWizard, OnFinish, Button*, void)
    {
        if ( isTravelingSuspended() )
            return;
        RoadmapWizardTravelSuspension aTravelGuard( *this );
        if (!prepareLeaveCurrentState(WizardTypes::eFinish))
        {
            return;
        }
        onFinish();
    }

    bool RoadmapWizard::prepareLeaveCurrentState( WizardTypes::CommitPageReason _eReason )
    {
        IWizardPageController* pController = getPageController( GetPage( getCurrentState() ) );
        ENSURE_OR_RETURN( pController != nullptr, "RoadmapWizard::prepareLeaveCurrentState: no controller for the current page!", true );
        return pController->commitPage( _eReason );
    }

    bool RoadmapWizard::skipBackwardUntil( WizardTypes::WizardState _nTargetState )
    {
        // allowed to leave the current page?
        if (!prepareLeaveCurrentState(WizardTypes::eTravelBackward))
            return false;

        // don't travel directly on m_xWizardImpl->aStateHistory, in case something goes wrong
        std::stack< WizardTypes::WizardState > aTravelVirtually = m_xWizardImpl->aStateHistory;
        std::stack< WizardTypes::WizardState > aOldStateHistory = m_xWizardImpl->aStateHistory;

        WizardTypes::WizardState nCurrentRollbackState = getCurrentState();
        while ( nCurrentRollbackState != _nTargetState )
        {
            DBG_ASSERT( !aTravelVirtually.empty(), "RoadmapWizard::skipBackwardUntil: this target state does not exist in the history!" );
            nCurrentRollbackState = aTravelVirtually.top();
            aTravelVirtually.pop();
        }
        m_xWizardImpl->aStateHistory = aTravelVirtually;
        if ( !ShowPage( _nTargetState ) )
        {
            m_xWizardImpl->aStateHistory = aOldStateHistory;
            return false;
        }
        return true;
    }

    bool RoadmapWizard::skipUntil( WizardTypes::WizardState _nTargetState )
    {
        WizardTypes::WizardState nCurrentState = getCurrentState();

        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( nCurrentState < _nTargetState ? WizardTypes::eTravelForward : WizardTypes::eTravelBackward ) )
            return false;

        // don't travel directly on m_xWizardImpl->aStateHistory, in case something goes wrong
        std::stack< WizardTypes::WizardState > aTravelVirtually = m_xWizardImpl->aStateHistory;
        std::stack< WizardTypes::WizardState > aOldStateHistory = m_xWizardImpl->aStateHistory;
        while ( nCurrentState != _nTargetState )
        {
            WizardTypes::WizardState nNextState = determineNextState( nCurrentState );
            if ( WZS_INVALID_STATE == nNextState )
            {
                OSL_FAIL( "RoadmapWizard::skipUntil: the given target state does not exist!" );
                return false;
            }

            // remember the skipped state in the history
            aTravelVirtually.push( nCurrentState );

            // get the next state
            nCurrentState = nNextState;
        }
        m_xWizardImpl->aStateHistory = aTravelVirtually;
        // show the target page
        if ( !ShowPage( nCurrentState ) )
        {
            // argh! prepareLeaveCurrentPage succeeded, determineNextState succeeded,
            // but ShowPage doesn't? Somebody behaves very strange here...
            OSL_FAIL( "RoadmapWizard::skipUntil: very unpolite..." );
            m_xWizardImpl->aStateHistory = aOldStateHistory;
            return false;
        }
        return true;
    }

    void RoadmapWizard::skip()
    {
        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( WizardTypes::eTravelForward ) )
            return;

        WizardTypes::WizardState nCurrentState = getCurrentState();
        WizardTypes::WizardState nNextState = determineNextState(nCurrentState);

        if (WZS_INVALID_STATE == nNextState)
            return;

        // remember the skipped state in the history
        m_xWizardImpl->aStateHistory.push(nCurrentState);

        // get the next state
        nCurrentState = nNextState;

        // show the (n+1)th page
        if (!ShowPage(nCurrentState))
        {
            // TODO: this leaves us in a state where we have no current page and an inconsistent state history.
            // Perhaps we should rollback the skipping here...
            OSL_FAIL("RoadmapWizard::skip: very unpolite...");
                // if somebody does a skip and then does not allow to leave...
                // (can't be a commit error, as we've already committed the current page. So if ShowPage fails here,
                // somebody behaves really strange...)
            return;
        }

        // all fine
    }

    bool RoadmapWizard::travelNext()
    {
        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( WizardTypes::eTravelForward ) )
            return false;

        // determine the next state to travel to
        WizardTypes::WizardState nCurrentState = getCurrentState();
        WizardTypes::WizardState nNextState = determineNextState(nCurrentState);
        if (WZS_INVALID_STATE == nNextState)
            return false;

        // the state history is used by the enterState method
        // all fine
        m_xWizardImpl->aStateHistory.push(nCurrentState);
        if (!ShowPage(nNextState))
        {
            m_xWizardImpl->aStateHistory.pop();
            return false;
        }

        return true;
    }

    bool RoadmapWizard::travelPrevious()
    {
        DBG_ASSERT(!m_xWizardImpl->aStateHistory.empty(), "RoadmapWizard::travelPrevious: have no previous page!");

        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( WizardTypes::eTravelBackward ) )
            return false;

        // the next state to switch to
        WizardTypes::WizardState nPreviousState = m_xWizardImpl->aStateHistory.top();

        // the state history is used by the enterState method
        m_xWizardImpl->aStateHistory.pop();
        // show this page
        if (!ShowPage(nPreviousState))
        {
            m_xWizardImpl->aStateHistory.push(nPreviousState);
            return false;
        }

        // all fine
        return true;
    }

    void  RoadmapWizard::removePageFromHistory( WizardTypes::WizardState nToRemove )
    {

        std::stack< WizardTypes::WizardState > aTemp;
        while(!m_xWizardImpl->aStateHistory.empty())
        {
            WizardTypes::WizardState nPreviousState = m_xWizardImpl->aStateHistory.top();
            m_xWizardImpl->aStateHistory.pop();
            if(nPreviousState != nToRemove)
                aTemp.push( nPreviousState );
            else
                break;
        }
        while(!aTemp.empty())
        {
            m_xWizardImpl->aStateHistory.push( aTemp.top() );
            aTemp.pop();
        }
    }

    void RoadmapWizard::enableAutomaticNextButtonState()
    {
        m_xWizardImpl->m_bAutoNextButtonState = true;
    }

    bool RoadmapWizard::isAutomaticNextButtonStateEnabled() const
    {
        return m_xWizardImpl->m_bAutoNextButtonState;
    }

    IMPL_LINK_NOARG(RoadmapWizard, OnPrevPage, Button*, void)
    {
        if ( isTravelingSuspended() )
            return;
        RoadmapWizardTravelSuspension aTravelGuard( *this );
        travelPrevious();
    }

    IMPL_LINK_NOARG(RoadmapWizard, OnNextPage, Button*, void)
    {
        if ( isTravelingSuspended() )
            return;
        RoadmapWizardTravelSuspension aTravelGuard( *this );
        travelNext();
    }

    IWizardPageController* RoadmapWizard::getPageController( TabPage* _pCurrentPage ) const
    {
        IWizardPageController* pController = dynamic_cast< IWizardPageController* >( _pCurrentPage );
        return pController;
    }

    void RoadmapWizard::getStateHistory( std::vector< WizardTypes::WizardState >& _out_rHistory )
    {
        std::stack< WizardTypes::WizardState > aHistoryCopy( m_xWizardImpl->aStateHistory );
        while ( !aHistoryCopy.empty() )
        {
            _out_rHistory.push_back( aHistoryCopy.top() );
            aHistoryCopy.pop();
        }
    }

    bool RoadmapWizard::isTravelingSuspended() const
    {
        return m_xWizardImpl->m_bTravelingSuspended;
    }

    void RoadmapWizard::suspendTraveling( AccessGuard )
    {
        DBG_ASSERT( !m_xWizardImpl->m_bTravelingSuspended, "RoadmapWizard::suspendTraveling: already suspended!" );
        m_xWizardImpl->m_bTravelingSuspended = true;
    }

    void RoadmapWizard::resumeTraveling( AccessGuard )
    {
        DBG_ASSERT( m_xWizardImpl->m_bTravelingSuspended, "RoadmapWizard::resumeTraveling: nothing to resume!" );
        m_xWizardImpl->m_bTravelingSuspended = false;
    }

    WizardMachine::WizardMachine(weld::Window* pParent, WizardButtonFlags nButtonFlags)
        : AssistantController(pParent, "vcl/ui/wizard.ui", "Wizard")
        , m_nCurState(0)
        , m_pFirstPage(nullptr)
        , m_xFinish(m_xAssistant->weld_widget_for_response(RET_OK))
        , m_xCancel(m_xAssistant->weld_widget_for_response(RET_CANCEL))
        , m_xNextPage(m_xAssistant->weld_widget_for_response(RET_YES))
        , m_xPrevPage(m_xAssistant->weld_widget_for_response(RET_NO))
        , m_xHelp(m_xAssistant->weld_widget_for_response(RET_HELP))
        , m_pImpl(new WizardMachineImplData)
    {
        implConstruct(nButtonFlags);
    }

    void WizardMachine::implConstruct(const WizardButtonFlags nButtonFlags)
    {
        m_pImpl->sTitleBase = m_xAssistant->get_title();

        // create the buttons according to the wizard button flags
        // the help button
        if (nButtonFlags & WizardButtonFlags::HELP)
            m_xHelp->show();
        else
            m_xHelp->hide();

        // the previous button
        if (nButtonFlags & WizardButtonFlags::PREVIOUS)
        {
            m_xPrevPage->set_help_id( HID_WIZARD_PREVIOUS );
            m_xPrevPage->show();

            m_xPrevPage->connect_clicked( LINK( this, WizardMachine, OnPrevPage ) );
        }
        else
            m_xPrevPage->hide();

        // the next button
        if (nButtonFlags & WizardButtonFlags::NEXT)
        {
            m_xNextPage->set_help_id( HID_WIZARD_NEXT );
            m_xNextPage->show();

            m_xNextPage->connect_clicked( LINK( this, WizardMachine, OnNextPage ) );
        }
        else
            m_xNextPage->hide();

        // the finish button
        if (nButtonFlags & WizardButtonFlags::FINISH)
        {
            m_xFinish->show();

            m_xFinish->connect_clicked( LINK( this, WizardMachine, OnFinish ) );
        }
        else
            m_xFinish->hide();

        // the cancel button
        if (nButtonFlags & WizardButtonFlags::CANCEL)
        {
            m_xCancel->show();
            m_xCancel->connect_clicked( LINK( this, WizardMachine, OnCancel ) );
        }
        else
            m_xCancel->hide();
    }

    WizardMachine::~WizardMachine()
    {
        if (m_pImpl)
        {
            while (m_pFirstPage)
            {
                VclPtr<TabPage> pPage = m_pFirstPage->mpPage;
                RemovePage(m_pFirstPage->mpPage);
                if (pPage)
                    pPage.disposeAndClear();
            }

            m_pImpl.reset();
        }
    }

    void WizardMachine::implUpdateTitle()
    {
        OUString sCompleteTitle(m_pImpl->sTitleBase);

        // append the page title
        TabPage* pCurrentPage = GetPage(getCurrentState());
        if ( pCurrentPage && !pCurrentPage->GetText().isEmpty() )
        {
            sCompleteTitle += " - " + pCurrentPage->GetText();
        }

        m_xAssistant->set_title(sCompleteTitle);
    }

    void WizardMachine::setTitleBase(const OUString& _rTitleBase)
    {
        m_pImpl->sTitleBase = _rTitleBase;
        implUpdateTitle();
    }

    TabPage* WizardMachine::GetOrCreatePage( const WizardTypes::WizardState i_nState )
    {
        if ( nullptr == GetPage( i_nState ) )
        {
            VclPtr<TabPage> pNewPage = createPage( i_nState );
            DBG_ASSERT( pNewPage, "WizardMachine::GetOrCreatePage: invalid new page (NULL)!" );

            // fill up the page sequence of our base class (with dummies)
            while ( m_pImpl->nFirstUnknownPage < i_nState )
            {
                AddPage( nullptr );
                ++m_pImpl->nFirstUnknownPage;
            }

            if ( m_pImpl->nFirstUnknownPage == i_nState )
            {
                // encountered this page number the first time
                AddPage( pNewPage );
                ++m_pImpl->nFirstUnknownPage;
            }
            else
                // already had this page - just change it
                SetPage( i_nState, pNewPage );
        }
        return GetPage( i_nState );
    }

    void WizardMachine::ActivatePage()
    {
        WizardTypes::WizardState nCurrentLevel = m_nCurState;
        GetOrCreatePage( nCurrentLevel );

        enterState( nCurrentLevel );
    }

    bool WizardMachine::DeactivatePage()
    {
        WizardTypes::WizardState nCurrentState = getCurrentState();
        return leaveState(nCurrentState);
    }

    void WizardMachine::defaultButton(WizardButtonFlags _nWizardButtonFlags)
    {
        // the new default button
        weld::Button* pNewDefButton = nullptr;
        if (_nWizardButtonFlags & WizardButtonFlags::FINISH)
            pNewDefButton = m_xFinish.get();
        if (_nWizardButtonFlags & WizardButtonFlags::NEXT)
            pNewDefButton = m_xNextPage.get();
        if (_nWizardButtonFlags & WizardButtonFlags::PREVIOUS)
            pNewDefButton = m_xPrevPage.get();
        if (_nWizardButtonFlags & WizardButtonFlags::HELP)
            pNewDefButton = m_xHelp.get();
        if (_nWizardButtonFlags & WizardButtonFlags::CANCEL)
            pNewDefButton = m_xCancel.get();

        if ( pNewDefButton )
            defaultButton( pNewDefButton );
        else
            m_xAssistant->recursively_unset_default_buttons();
    }

    void WizardMachine::defaultButton(weld::Button* _pNewDefButton)
    {
        // loop through all (direct and indirect) descendants which participate in our tabbing order, and
        // reset the WB_DEFBUTTON for every window which is a button
        m_xAssistant->recursively_unset_default_buttons();

        // set its new style
        if (_pNewDefButton)
            _pNewDefButton->set_has_default(true);
    }

    void WizardMachine::enableButtons(WizardButtonFlags _nWizardButtonFlags, bool _bEnable)
    {
        if (_nWizardButtonFlags & WizardButtonFlags::FINISH)
            m_xFinish->set_sensitive(_bEnable);
        if (_nWizardButtonFlags & WizardButtonFlags::NEXT)
            m_xNextPage->set_sensitive(_bEnable);
        if (_nWizardButtonFlags & WizardButtonFlags::PREVIOUS)
            m_xPrevPage->set_sensitive(_bEnable);
        if (_nWizardButtonFlags & WizardButtonFlags::HELP)
            m_xHelp->set_sensitive(_bEnable);
        if (_nWizardButtonFlags & WizardButtonFlags::CANCEL)
            m_xCancel->set_sensitive(_bEnable);
    }

    void WizardMachine::enterState(WizardTypes::WizardState _nState)
    {
        // tell the page
        IWizardPageController* pController = getPageController( GetPage( _nState ) );
        OSL_ENSURE( pController, "WizardMachine::enterState: no controller for the given page!" );
        if ( pController )
            pController->initializePage();

        if ( isAutomaticNextButtonStateEnabled() )
            enableButtons( WizardButtonFlags::NEXT, canAdvance() );

        enableButtons( WizardButtonFlags::PREVIOUS, !m_pImpl->aStateHistory.empty() );

        // set the new title - it depends on the current page (i.e. state)
        implUpdateTitle();
    }

    bool WizardMachine::leaveState(WizardTypes::WizardState)
    {
        // no need to ask the page here.
        // If we reach this point, we already gave the current page the chance to commit it's data,
        // and it was allowed to commit it's data

        return true;
    }

    bool WizardMachine::onFinish()
    {
        return Finish(RET_OK);
    }

    IMPL_LINK_NOARG(WizardMachine, OnFinish, weld::Button&, void)
    {
        if ( isTravelingSuspended() )
            return;
        WizardTravelSuspension aTravelGuard( *this );
        if (!prepareLeaveCurrentState(WizardTypes::eFinish))
        {
            return;
        }
        onFinish();
    }

    IMPL_LINK_NOARG(WizardMachine, OnCancel, weld::Button&, void)
    {
        m_xAssistant->response(RET_CANCEL);
    }

    WizardTypes::WizardState WizardMachine::determineNextState(WizardTypes::WizardState _nCurrentState ) const
    {
        return _nCurrentState + 1;
    }

    bool WizardMachine::prepareLeaveCurrentState( WizardTypes::CommitPageReason _eReason )
    {
        IWizardPageController* pController = getPageController( GetPage( getCurrentState() ) );
        ENSURE_OR_RETURN( pController != nullptr, "WizardMachine::prepareLeaveCurrentState: no controller for the current page!", true );
        return pController->commitPage( _eReason );
    }

    bool WizardMachine::skipBackwardUntil(WizardTypes::WizardState _nTargetState)
    {
        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( WizardTypes::eTravelBackward ) )
            return false;

        // don't travel directly on m_pImpl->aStateHistory, in case something goes wrong
        std::stack< WizardTypes::WizardState > aTravelVirtually = m_pImpl->aStateHistory;
        std::stack< WizardTypes::WizardState > aOldStateHistory = m_pImpl->aStateHistory;

        WizardTypes::WizardState nCurrentRollbackState = getCurrentState();
        while ( nCurrentRollbackState != _nTargetState )
        {
            DBG_ASSERT( !aTravelVirtually.empty(), "WizardMachine::skipBackwardUntil: this target state does not exist in the history!" );
            nCurrentRollbackState = aTravelVirtually.top();
            aTravelVirtually.pop();
        }
        m_pImpl->aStateHistory = aTravelVirtually;
        if ( !ShowPage( _nTargetState ) )
        {
            m_pImpl->aStateHistory = aOldStateHistory;
            return false;
        }
        return true;
    }

    bool WizardMachine::skipUntil( WizardTypes::WizardState _nTargetState )
    {
        WizardTypes::WizardState nCurrentState = getCurrentState();

        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( nCurrentState < _nTargetState ? WizardTypes::eTravelForward : WizardTypes::eTravelBackward ) )
            return false;

        // don't travel directly on m_pImpl->aStateHistory, in case something goes wrong
        std::stack< WizardTypes::WizardState > aTravelVirtually = m_pImpl->aStateHistory;
        std::stack< WizardTypes::WizardState > aOldStateHistory = m_pImpl->aStateHistory;
        while ( nCurrentState != _nTargetState )
        {
            WizardTypes::WizardState nNextState = determineNextState( nCurrentState );
            if ( WZS_INVALID_STATE == nNextState )
            {
                OSL_FAIL( "WizardMachine::skipUntil: the given target state does not exist!" );
                return false;
            }

            // remember the skipped state in the history
            aTravelVirtually.push( nCurrentState );

            // get the next state
            nCurrentState = nNextState;
        }
        m_pImpl->aStateHistory = aTravelVirtually;
        // show the target page
        if ( !ShowPage( nCurrentState ) )
        {
            // argh! prepareLeaveCurrentPage succeeded, determineNextState succeeded,
            // but ShowPage doesn't? Somebody behaves very strange here...
            OSL_FAIL( "WizardMachine::skipUntil: very unpolite..." );
            m_pImpl->aStateHistory = aOldStateHistory;
            return false;
        }
        return true;
    }

    void WizardMachine::skip()
    {
        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( WizardTypes::eTravelForward ) )
            return;

        WizardTypes::WizardState nCurrentState = getCurrentState();
        WizardTypes::WizardState nNextState = determineNextState(nCurrentState);

        if (WZS_INVALID_STATE == nNextState)
            return;

        // remember the skipped state in the history
        m_pImpl->aStateHistory.push(nCurrentState);

        // get the next state
        nCurrentState = nNextState;

        // show the (n+1)th page
        if (!ShowPage(nCurrentState))
        {
            // TODO: this leaves us in a state where we have no current page and an inconsistent state history.
            // Perhaps we should rollback the skipping here...
            OSL_FAIL("RoadmapWizard::skip: very unpolite...");
                // if somebody does a skip and then does not allow to leave...
                // (can't be a commit error, as we've already committed the current page. So if ShowPage fails here,
                // somebody behaves really strange ...)
            return;
        }

        // all fine
    }

    bool WizardMachine::travelNext()
    {
        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( WizardTypes::eTravelForward ) )
            return false;

        // determine the next state to travel to
        WizardTypes::WizardState nCurrentState = getCurrentState();
        WizardTypes::WizardState nNextState = determineNextState(nCurrentState);
        if (WZS_INVALID_STATE == nNextState)
            return false;

        // the state history is used by the enterState method
        // all fine
        m_pImpl->aStateHistory.push(nCurrentState);
        if (!ShowPage(nNextState))
        {
            m_pImpl->aStateHistory.pop();
            return false;
        }

        return true;
    }

    bool WizardMachine::ShowPage(WizardTypes::WizardState nState)
    {
        if (DeactivatePage())
        {
            TabPage* pOldTabPage = m_xCurTabPage;

            m_nCurState = nState;
            ActivatePage();

            if (pOldTabPage)
                pOldTabPage->DeactivatePage();

            m_xAssistant->set_current_page(OString::number(nState));

            m_xCurTabPage = GetPage(m_nCurState);
            m_xCurTabPage->ActivatePage();

            return true;
        }
        return false;
    }

    bool WizardMachine::ShowNextPage()
    {
        return ShowPage(m_nCurState + 1);
    }

    bool WizardMachine::ShowPrevPage()
    {
        if (!m_nCurState)
            return false;
        return ShowPage(m_nCurState - 1);
    }

    bool WizardMachine::travelPrevious()
    {
        DBG_ASSERT(!m_pImpl->aStateHistory.empty(), "WizardMachine::travelPrevious: have no previous page!");

        // allowed to leave the current page?
        if ( !prepareLeaveCurrentState( WizardTypes::eTravelBackward ) )
            return false;

        // the next state to switch to
        WizardTypes::WizardState nPreviousState = m_pImpl->aStateHistory.top();

        // the state history is used by the enterState method
        m_pImpl->aStateHistory.pop();
        // show this page
        if (!ShowPage(nPreviousState))
        {
            m_pImpl->aStateHistory.push(nPreviousState);
            return false;
        }

        // all fine
        return true;
    }


    void  WizardMachine::removePageFromHistory( WizardTypes::WizardState nToRemove )
    {

        std::stack< WizardTypes::WizardState > aTemp;
        while(!m_pImpl->aStateHistory.empty())
        {
            WizardTypes::WizardState nPreviousState = m_pImpl->aStateHistory.top();
            m_pImpl->aStateHistory.pop();
            if(nPreviousState != nToRemove)
                aTemp.push( nPreviousState );
            else
                break;
        }
        while(!aTemp.empty())
        {
            m_pImpl->aStateHistory.push( aTemp.top() );
            aTemp.pop();
        }
    }


    void WizardMachine::enableAutomaticNextButtonState()
    {
        m_pImpl->m_bAutoNextButtonState = true;
    }


    bool WizardMachine::isAutomaticNextButtonStateEnabled() const
    {
        return m_pImpl->m_bAutoNextButtonState;
    }

    IMPL_LINK_NOARG(WizardMachine, OnPrevPage, weld::Button&, void)
    {
        if ( isTravelingSuspended() )
            return;
        WizardTravelSuspension aTravelGuard( *this );
        travelPrevious();
    }

    IMPL_LINK_NOARG(WizardMachine, OnNextPage, weld::Button&, void)
    {
        if ( isTravelingSuspended() )
            return;
        WizardTravelSuspension aTravelGuard( *this );
        travelNext();
    }

    IWizardPageController* WizardMachine::getPageController( TabPage* _pCurrentPage ) const
    {
        IWizardPageController* pController = dynamic_cast< IWizardPageController* >( _pCurrentPage );
        return pController;
    }

    void WizardMachine::getStateHistory( std::vector< WizardTypes::WizardState >& _out_rHistory )
    {
        std::stack< WizardTypes::WizardState > aHistoryCopy( m_pImpl->aStateHistory );
        while ( !aHistoryCopy.empty() )
        {
            _out_rHistory.push_back( aHistoryCopy.top() );
            aHistoryCopy.pop();
        }
    }

    bool WizardMachine::canAdvance() const
    {
        return WZS_INVALID_STATE != determineNextState( getCurrentState() );
    }

    void WizardMachine::updateTravelUI()
    {
        const IWizardPageController* pController = getPageController( GetPage( getCurrentState() ) );
        OSL_ENSURE( pController != nullptr, "RoadmapWizard::updateTravelUI: no controller for the current page!" );

        bool bCanAdvance =
                ( !pController || pController->canAdvance() )   // the current page allows to advance
            &&  canAdvance();                                   // the dialog as a whole allows to advance
        enableButtons( WizardButtonFlags::NEXT, bCanAdvance );
    }

    bool WizardMachine::isTravelingSuspended() const
    {
        return m_pImpl->m_bTravelingSuspended;
    }

    void WizardMachine::suspendTraveling( AccessGuard )
    {
        DBG_ASSERT( !m_pImpl->m_bTravelingSuspended, "WizardMachine::suspendTraveling: already suspended!" );
        m_pImpl->m_bTravelingSuspended = true;
    }

    void WizardMachine::resumeTraveling( AccessGuard )
    {
        DBG_ASSERT( m_pImpl->m_bTravelingSuspended, "WizardMachine::resumeTraveling: nothing to resume!" );
        m_pImpl->m_bTravelingSuspended = false;
    }

    bool WizardMachine::Finish(short nResult)
    {
        if ( DeactivatePage() )
        {
            if (m_xCurTabPage)
                m_xCurTabPage->DeactivatePage();

            m_xAssistant->response(nResult);
            return true;
        }
        else
            return false;
    }

    void WizardMachine::AddPage( TabPage* pPage )
    {
        ImplWizPageData* pNewPageData = new ImplWizPageData;
        pNewPageData->mpNext    = nullptr;
        pNewPageData->mpPage    = pPage;

        if ( !m_pFirstPage )
            m_pFirstPage = pNewPageData;
        else
        {
            ImplWizPageData* pPageData = m_pFirstPage;
            while ( pPageData->mpNext )
                pPageData = pPageData->mpNext;
            pPageData->mpNext = pNewPageData;
        }
    }

    void WizardMachine::RemovePage( TabPage* pPage )
    {
        ImplWizPageData*  pPrevPageData = nullptr;
        ImplWizPageData*  pPageData = m_pFirstPage;
        while ( pPageData )
        {
            if ( pPageData->mpPage == pPage )
            {
                if (pPrevPageData)
                    pPrevPageData->mpNext = pPageData->mpNext;
                else
                    m_pFirstPage = pPageData->mpNext;
                if (pPage == m_xCurTabPage)
                    m_xCurTabPage.clear();
                delete pPageData;
                return;
            }

            pPrevPageData = pPageData;
            pPageData = pPageData->mpNext;
        }

        OSL_FAIL( "WizardMachine::RemovePage() - Page not in list" );
    }

    void WizardMachine::SetPage(WizardTypes::WizardState nLevel, TabPage* pPage)
    {
        sal_uInt16              nTempLevel = 0;
        ImplWizPageData*    pPageData = m_pFirstPage;
        while ( pPageData )
        {
            if ( (nTempLevel == nLevel) || !pPageData->mpNext )
                break;

            nTempLevel++;
            pPageData = pPageData->mpNext;
        }

        if ( pPageData )
        {
            if ( pPageData->mpPage == m_xCurTabPage )
                m_xCurTabPage = nullptr;
            pPageData->mpPage = pPage;
        }
    }

    TabPage* WizardMachine::GetPage(WizardTypes::WizardState nLevel) const
    {
        sal_uInt16 nTempLevel = 0;

        for (ImplWizPageData* pPageData = m_pFirstPage; pPageData;
             pPageData = pPageData->mpNext)
        {
            if ( nTempLevel == nLevel )
                return pPageData->mpPage;
            nTempLevel++;
        }

        return nullptr;
    }
}   // namespace svt


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
