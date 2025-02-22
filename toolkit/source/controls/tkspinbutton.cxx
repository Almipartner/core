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

#include <com/sun/star/awt/ScrollBarOrientation.hpp>
#include <com/sun/star/awt/XSpinValue.hpp>
#include <com/sun/star/awt/XAdjustmentListener.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <comphelper/uno3.hxx>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <toolkit/controls/unocontrolmodel.hxx>
#include <toolkit/controls/unocontrolbase.hxx>
#include <toolkit/helper/macros.hxx>
#include <toolkit/helper/property.hxx>

#include <helper/unopropertyarrayhelper.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;

namespace {

class UnoSpinButtonModel : public UnoControlModel
{
protected:
    css::uno::Any      ImplGetDefaultValue( sal_uInt16 nPropId ) const override;
    ::cppu::IPropertyArrayHelper&   SAL_CALL getInfoHelper() override;

public:
    explicit UnoSpinButtonModel( const css::uno::Reference< css::uno::XComponentContext >& i_factory );

    rtl::Reference<UnoControlModel> Clone() const override { return new UnoSpinButtonModel( *this ); }

    // XMultiPropertySet
    css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;

    // XPersistObject
    OUString SAL_CALL getServiceName() override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName(  ) override;
    css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
};


//= UnoSpinButtonControl


typedef ::cppu::ImplHelper2 <   css::awt::XAdjustmentListener
                            ,   css::awt::XSpinValue
                            >   UnoSpinButtonControl_Base;

class UnoSpinButtonControl :public UnoControlBase
                           ,public UnoSpinButtonControl_Base
{
private:
    AdjustmentListenerMultiplexer maAdjustmentListeners;

public:
                                UnoSpinButtonControl();
    OUString             GetComponentServiceName() override;

    DECLARE_UNO3_AGG_DEFAULTS( UnoSpinButtonControl, UnoControlBase )
    css::uno::Any  SAL_CALL queryAggregation( const css::uno::Type & rType ) override;

    void SAL_CALL createPeer( const css::uno::Reference< css::awt::XToolkit >& Toolkit, const css::uno::Reference< css::awt::XWindowPeer >& Parent ) override;
    void SAL_CALL disposing( const css::lang::EventObject& Source ) override { UnoControlBase::disposing( Source ); }
    void SAL_CALL dispose(  ) override;

    // XTypeProvider
    DECLARE_XTYPEPROVIDER()

    // XAdjustmentListener
    void SAL_CALL adjustmentValueChanged( const css::awt::AdjustmentEvent& rEvent ) override;

    // XSpinValue
    virtual void SAL_CALL addAdjustmentListener( const css::uno::Reference< css::awt::XAdjustmentListener >& listener ) override;
    virtual void SAL_CALL removeAdjustmentListener( const css::uno::Reference< css::awt::XAdjustmentListener >& listener ) override;
    virtual void SAL_CALL setValue( sal_Int32 value ) override;
    virtual void SAL_CALL setValues( sal_Int32 minValue, sal_Int32 maxValue, sal_Int32 currentValue ) override;
    virtual sal_Int32 SAL_CALL getValue(  ) override;
    virtual void SAL_CALL setMinimum( sal_Int32 minValue ) override;
    virtual void SAL_CALL setMaximum( sal_Int32 maxValue ) override;
    virtual sal_Int32 SAL_CALL getMinimum(  ) override;
    virtual sal_Int32 SAL_CALL getMaximum(  ) override;
    virtual void SAL_CALL setSpinIncrement( sal_Int32 spinIncrement ) override;
    virtual sal_Int32 SAL_CALL getSpinIncrement(  ) override;
    virtual void SAL_CALL setOrientation( sal_Int32 orientation ) override;
    virtual sal_Int32 SAL_CALL getOrientation(  ) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName(  ) override;
    css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
};


    //= UnoSpinButtonModel


    UnoSpinButtonModel::UnoSpinButtonModel( const css::uno::Reference< css::uno::XComponentContext >& i_factory )
        :UnoControlModel( i_factory )
    {
        ImplRegisterProperty( BASEPROPERTY_BACKGROUNDCOLOR );
        ImplRegisterProperty( BASEPROPERTY_BORDER );
        ImplRegisterProperty( BASEPROPERTY_BORDERCOLOR );
        ImplRegisterProperty( BASEPROPERTY_DEFAULTCONTROL );
        ImplRegisterProperty( BASEPROPERTY_ENABLED );
        ImplRegisterProperty( BASEPROPERTY_ENABLEVISIBLE );
        ImplRegisterProperty( BASEPROPERTY_HELPTEXT );
        ImplRegisterProperty( BASEPROPERTY_HELPURL );
        ImplRegisterProperty( BASEPROPERTY_ORIENTATION );
        ImplRegisterProperty( BASEPROPERTY_PRINTABLE );
        ImplRegisterProperty( BASEPROPERTY_REPEAT );
        ImplRegisterProperty( BASEPROPERTY_REPEAT_DELAY );
        ImplRegisterProperty( BASEPROPERTY_SYMBOL_COLOR );
        ImplRegisterProperty( BASEPROPERTY_SPINVALUE );
        ImplRegisterProperty( BASEPROPERTY_SPINVALUE_MIN );
        ImplRegisterProperty( BASEPROPERTY_SPINVALUE_MAX );
        ImplRegisterProperty( BASEPROPERTY_SPININCREMENT );
        ImplRegisterProperty( BASEPROPERTY_TABSTOP );
        ImplRegisterProperty( BASEPROPERTY_WRITING_MODE );
        ImplRegisterProperty( BASEPROPERTY_CONTEXT_WRITING_MODE );
    }


    OUString UnoSpinButtonModel::getServiceName( )
    {
        return "com.sun.star.awt.UnoControlSpinButtonModel";
    }


    Any UnoSpinButtonModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
    {
        switch ( nPropId )
        {
        case BASEPROPERTY_DEFAULTCONTROL:
            return makeAny( OUString("com.sun.star.awt.UnoControlSpinButton") );

        case BASEPROPERTY_BORDER:
            return makeAny( sal_Int16(0) );

        case BASEPROPERTY_REPEAT:
            return makeAny( true );

        default:
            return UnoControlModel::ImplGetDefaultValue( nPropId );
        }
    }


    ::cppu::IPropertyArrayHelper& UnoSpinButtonModel::getInfoHelper()
    {
        static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
        return aHelper;
    }


    Reference< XPropertySetInfo > UnoSpinButtonModel::getPropertySetInfo(  )
    {
        static Reference< XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
        return xInfo;
    }


    OUString SAL_CALL UnoSpinButtonModel::getImplementationName(  )
    {
        return "stardiv.Toolkit.UnoSpinButtonModel";
    }


    Sequence< OUString > SAL_CALL UnoSpinButtonModel::getSupportedServiceNames()
    {
        Sequence< OUString > aServices( UnoControlModel::getSupportedServiceNames() );
        aServices.realloc( aServices.getLength() + 1 );
        aServices[ aServices.getLength() - 1 ] = "com.sun.star.awt.UnoControlSpinButtonModel";
        return aServices;
    }


    //= UnoSpinButtonControl


    UnoSpinButtonControl::UnoSpinButtonControl()
        :UnoControlBase()
        ,maAdjustmentListeners( *this )
    {
    }


    OUString UnoSpinButtonControl::GetComponentServiceName()
    {
        return "SpinButton";
    }


    Any UnoSpinButtonControl::queryAggregation( const Type & rType )
    {
        Any aRet = UnoControlBase::queryAggregation( rType );
        if ( !aRet.hasValue() )
            aRet = UnoSpinButtonControl_Base::queryInterface( rType );
        return aRet;
    }


    IMPLEMENT_FORWARD_XTYPEPROVIDER2( UnoSpinButtonControl, UnoControlBase, UnoSpinButtonControl_Base )


    void UnoSpinButtonControl::dispose()
    {
        ::osl::ClearableMutexGuard aGuard( GetMutex() );
        if ( maAdjustmentListeners.getLength() )
        {
            Reference< XSpinValue > xSpinnable( getPeer(), UNO_QUERY );
            if ( xSpinnable.is() )
                xSpinnable->removeAdjustmentListener( this );

            EventObject aDisposeEvent;
            aDisposeEvent.Source = *this;

            aGuard.clear();
            maAdjustmentListeners.disposeAndClear( aDisposeEvent );
        }

        UnoControl::dispose();
    }


    OUString SAL_CALL UnoSpinButtonControl::getImplementationName(  )
    {
        return "stardiv.Toolkit.UnoSpinButtonControl";
    }


    Sequence< OUString > SAL_CALL UnoSpinButtonControl::getSupportedServiceNames()
    {
        Sequence< OUString > aServices( UnoControlBase::getSupportedServiceNames() );
        aServices.realloc( aServices.getLength() + 1 );
        aServices[ aServices.getLength() - 1 ] = "com.sun.star.awt.UnoControlSpinButton";
        return aServices;
    }


    void UnoSpinButtonControl::createPeer( const Reference< XToolkit > & rxToolkit, const Reference< XWindowPeer >  & rParentPeer )
    {
        UnoControl::createPeer( rxToolkit, rParentPeer );

        Reference < XSpinValue > xSpinnable( getPeer(), UNO_QUERY );
        if ( xSpinnable.is() )
            xSpinnable->addAdjustmentListener( this );
    }


    void UnoSpinButtonControl::adjustmentValueChanged( const AdjustmentEvent& rEvent )
    {
        switch ( rEvent.Type )
        {
            case AdjustmentType_ADJUST_LINE:
            case AdjustmentType_ADJUST_PAGE:
            case AdjustmentType_ADJUST_ABS:
                ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPINVALUE ), makeAny( rEvent.Value ), false );
                break;
            default:
                OSL_FAIL( "UnoSpinButtonControl::adjustmentValueChanged - unknown Type" );
        }

        if ( maAdjustmentListeners.getLength() )
        {
            AdjustmentEvent aEvent( rEvent );
            aEvent.Source = *this;
            maAdjustmentListeners.adjustmentValueChanged( aEvent );
        }
    }


    void UnoSpinButtonControl::addAdjustmentListener( const Reference< XAdjustmentListener > & listener )
    {
        ::osl::MutexGuard aGuard( GetMutex() );
        maAdjustmentListeners.addInterface( listener );
    }


    void UnoSpinButtonControl::removeAdjustmentListener( const Reference< XAdjustmentListener > & listener )
    {
        ::osl::MutexGuard aGuard( GetMutex() );
        maAdjustmentListeners.removeInterface( listener );
    }


    void SAL_CALL UnoSpinButtonControl::setValue( sal_Int32 value )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPINVALUE ), makeAny( value ), true );
    }


    void SAL_CALL UnoSpinButtonControl::setValues( sal_Int32 minValue, sal_Int32 maxValue, sal_Int32 currentValue )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPINVALUE_MIN ), makeAny( minValue ), true );
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPINVALUE_MAX ), makeAny( maxValue ), true );
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPINVALUE ), makeAny( currentValue ), true );
    }


    sal_Int32 SAL_CALL UnoSpinButtonControl::getValue(  )
    {
        ::osl::MutexGuard aGuard( GetMutex() );
        sal_Int32 nValue = 0;

        Reference< XSpinValue > xSpinnable( getPeer(), UNO_QUERY );
        if ( xSpinnable.is() )
            nValue = xSpinnable->getValue();

        return nValue;
    }


    void SAL_CALL UnoSpinButtonControl::setMinimum( sal_Int32 minValue )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPINVALUE_MIN ), makeAny( minValue ), true );
    }


    void SAL_CALL UnoSpinButtonControl::setMaximum( sal_Int32 maxValue )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPINVALUE_MAX ), makeAny( maxValue ), true );
    }


    sal_Int32 SAL_CALL UnoSpinButtonControl::getMinimum(  )
    {
        ::osl::MutexGuard aGuard( GetMutex() );
        sal_Int32 nMin = 0;

        Reference< XSpinValue > xSpinnable( getPeer(), UNO_QUERY );
        if ( xSpinnable.is() )
            nMin = xSpinnable->getMinimum();

        return nMin;
    }


    sal_Int32 SAL_CALL UnoSpinButtonControl::getMaximum(  )
    {
        ::osl::MutexGuard aGuard( GetMutex() );
        sal_Int32 nMax = 0;

        Reference< XSpinValue > xSpinnable( getPeer(), UNO_QUERY );
        if ( xSpinnable.is() )
            nMax = xSpinnable->getMaximum();

        return nMax;
    }


    void SAL_CALL UnoSpinButtonControl::setSpinIncrement( sal_Int32 spinIncrement )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SPININCREMENT ), makeAny( spinIncrement ), true );
    }


    sal_Int32 SAL_CALL UnoSpinButtonControl::getSpinIncrement(  )
    {
        ::osl::MutexGuard aGuard( GetMutex() );
        sal_Int32 nIncrement = 0;

        Reference< XSpinValue > xSpinnable( getPeer(), UNO_QUERY );
        if ( xSpinnable.is() )
            nIncrement = xSpinnable->getSpinIncrement();

        return nIncrement;
    }


    void SAL_CALL UnoSpinButtonControl::setOrientation( sal_Int32 orientation )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_ORIENTATION ), makeAny( orientation ), true );
    }


    sal_Int32 SAL_CALL UnoSpinButtonControl::getOrientation(  )
    {
        ::osl::MutexGuard aGuard( GetMutex() );
        sal_Int32 nOrientation = ScrollBarOrientation::HORIZONTAL;

        Reference< XSpinValue > xSpinnable( getPeer(), UNO_QUERY );
        if ( xSpinnable.is() )
            nOrientation = xSpinnable->getOrientation();

        return nOrientation;
    }

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoSpinButtonModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoSpinButtonModel(context));
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoSpinButtonControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoSpinButtonControl());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
