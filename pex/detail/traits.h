#pragma once


namespace pex
{


namespace detail
{


struct TraitsTest {};


template<typename T>
concept HasDerived = requires { typename T::template Derived<TraitsTest>; };


template<typename T>
concept HasDerivedValue = requires
{
    typename T::template DerivedValue<TraitsTest>;
};


template<typename T, typename Base>
concept HasModelTemplate = requires { typename T::template Model<Base>; };


template<typename T>
concept DeclaresModel = requires { typename T::Model; };


template<typename T, typename Base>
concept HasControlTemplate = requires { typename T::template Control<Base>; };


template<typename T, typename Base>
concept HasMuxTemplate = requires { typename T::template Mux<Base>; };


template<typename T, typename Base>
concept HasFollowTemplate = requires { typename T::template Follow<Base>; };


template<typename T>
concept HasModelUserBase = requires { typename T::ModelUserBase; };


template<typename T>
concept HasControlUserBase = requires { typename T::ControlUserBase; };


template<typename T, typename Base>
concept HasPlainTemplate = requires { typename T::template Plain<Base>; };


template<typename T>
concept HasPlain = requires { typename T::Plain; };


} // end namespace detail


} // end namespace pex
