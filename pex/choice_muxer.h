#pragma once


#include <pex/optional_select.h>
#include <pex/group.h>
#include <pex/endpoint.h>
#include <pex/interface.h>


namespace pex
{


template<typename T>
struct ChoiceMuxerFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::key, "key"),
        fields::Field(&T::select, "select"));
};


static_assert(IsMakeOptionalSelect<MakeOptionalSelect<int, GetAndSetTag>>);


template<typename ChoiceMaker>
struct ChoiceMuxerTemplate
{
    using Key = typename ChoiceMaker::Key;
    using Type = typename ChoiceMaker::Type;

    template<template<typename> typename T>
    struct Template
    {
        T<Key> key;
        T<MakeOptionalSelect<Type, GetAndSetTag>> select;

        static constexpr auto fields = ChoiceMuxerFields<Template>::fields;
        static constexpr auto fieldsTypeName = "ChoiceMuxer";
    };
};


template<typename ChoiceMaker>
struct ChoiceMuxerCustom
{
    template<typename Base>
    struct Model: public Base
    {
    public:
        using Key = typename ChoiceMaker::Key;
        using Type = typename ChoiceMaker::Type;
        using ChoiceMap = typename ChoiceMaker::ChoiceMap;

        Model()
            :
            Base{},
            choiceMap_{},
            keyEndpoint_(this, this->key, &Model::OnKey_)
        {
            this->SetChoiceMap(ChoiceMaker::GetChoiceMap());
            this->select.SetSelection(0);
        }

        void SetChoiceMap(const ChoiceMap &choiceMap)
        {
            if (choiceMap.empty())
            {
                throw std::logic_error("ChoiceMap cannot be empty");
            }

            this->choiceMap_ = choiceMap;

            if (this->choiceMap_.count(this->key.Get()))
            {
                // The selected key exists in the new map.
                // Update the choices.
                this->select.SetChoices(this->choiceMap_[this->key.Get()]);
            }
            else
            {
                if (this->choiceMap_.empty())
                {
                    this->select.SetChoices({});
                }
                else
                {
                    auto firstEntry = *this->choiceMap_.begin();

                    this->key.Set(firstEntry.first);
                    this->select.SetChoices(firstEntry.second);
                }
            }
        }

        // Allow read-only access by reference.
        const ChoiceMap & GetChoiceMap() const
        {
            return this->choiceMap_;
        }

    private:
        void OnKey_(Argument<Key> key_)
        {
            if (this->choiceMap_.count(key_) == 0)
            {
                // Set the choices to empty.
                // This automatically sets the selection and value to
                // std::nullopt.
                this->select.SetChoices({});

                assert(!this->select.GetSelectedIndex().has_value());
                assert(!this->select.Get().has_value());

                return;
            }

            this->select.SetChoices(this->choiceMap_[key_]);
        }

    private:
        ChoiceMap choiceMap_;

        using KeyEndpoint = pex::Endpoint<Model, decltype(Model::key)>;
        KeyEndpoint keyEndpoint_;
    };
};


template<typename ChoiceMaker>
using ChoiceMuxerGroup =
    pex::Group
    <
        ChoiceMuxerFields,
        ChoiceMuxerTemplate<ChoiceMaker>::template Template,
        ChoiceMuxerCustom<ChoiceMaker>
    >;

template<typename ChoiceMaker>
using ChoiceMuxerModel = typename ChoiceMuxerGroup<ChoiceMaker>::Model;

template<typename ChoiceMaker>
using ChoiceMuxerControl =
    typename ChoiceMuxerGroup<ChoiceMaker>::DefaultControl;

template<typename ChoiceMaker>
using ChoiceMuxer = typename ChoiceMuxerGroup<ChoiceMaker>::Plain;


} // end namespace pex
