#pragma once


#include <ostream>
#include <fields/describe.h>
#include "pex/detail/poly_detail.h"


namespace pex
{


namespace poly
{


// Interface that will be implemented for us in DerivedValue.
template
<
    typename Json_,
    typename Base,
    typename ModelBase_ = detail::DefaultModelBase
>
class PolyBase
{
public:
    using Json = Json_;
    using ModelBase = ModelBase_;

    virtual ~PolyBase() {}

    virtual std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const = 0;

    virtual Json Unstructure() const = 0;
    virtual bool operator==(const Base &) const = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual std::shared_ptr<Base> Copy() const = 0;

    void ReportAddress(const std::string &message) const
    {
        std::cout << message << ": " << this->GetTypeName() << " @ "
            << this << std::endl;
    }

    static std::shared_ptr<Base> Structure(const Json &jsonValues)
    {
        std::string typeName = jsonValues["type"];

        auto & creatorsByTypeName = PolyBase::CreatorsByTypeName_();

        if (1 != creatorsByTypeName.count(typeName))
        {
            throw std::runtime_error("Unregistered derived type: " + typeName);
        }

        return creatorsByTypeName[typeName](jsonValues);
    }

    bool CheckModel(ModelBase *modelBase) const
    {
        auto & modelCheckersByTypeName = PolyBase::ModelCheckersByTypeName_();
        auto typeName = std::string(this->GetTypeName());

        if (1 != modelCheckersByTypeName.count(typeName))
        {
            throw std::runtime_error("Unregistered model type: " + typeName);
        }

        return modelCheckersByTypeName[typeName](modelBase);
    }

    std::unique_ptr<ModelBase> CreateModel() const
    {
        auto & modelCreatorsByTypeName = PolyBase::ModelCreatorsByTypeName_();
        auto typeName = std::string(this->GetTypeName());

        if (1 != modelCreatorsByTypeName.count(typeName))
        {
            throw std::runtime_error("Unregistered model type: " + typeName);
        }

        return modelCreatorsByTypeName[typeName]();
    }

    static constexpr auto polyTypeName = "PolyBase";

    using CreatorFunction =
        std::function<std::shared_ptr<Base> (const Json &jsonValues)>;

    using CheckModelFunction =
        std::function<bool (ModelBase *base)>;

    using CreateModelFunction =
        std::function<std::unique_ptr<ModelBase> ()>;

    template<typename Derived>
    static void RegisterDerived(const std::string &key)
    {
        /*
        static_assert(
            fields::HasFieldsTypeName<Derived>,
            "Derived types must define a unique fieldsTypeName");

        auto key = std::string(Derived::fieldsTypeName);
        */

        if (key.empty())
        {
            throw std::logic_error("key is empty");
        }

        auto & creatorsByTypeName = PolyBase::CreatorsByTypeName_();

        if (1 == creatorsByTypeName.count(key))
        {
            throw std::logic_error(
                "Each Derived type must be registered only once.");
        }

        creatorsByTypeName[key] =
            [](const Json &jsonValues) -> std::shared_ptr<Base>
            {
                return std::make_shared<Derived>(
                    fields::Restructure<Derived>(jsonValues));
            };
    }

    template<typename Model>
    static void RegisterModel(const std::string &key)
    {
        static_assert(std::is_base_of_v<ModelBase, Model>);

        if (key.empty())
        {
            throw std::logic_error("key is empty");
        }

        auto & modelCheckerByTypeName = PolyBase::ModelCheckersByTypeName_();

        if (1 == modelCheckerByTypeName.count(key))
        {
            throw std::logic_error(
                "Each Derived type must be registered only once.");
        }

        modelCheckerByTypeName[key] =
            [](ModelBase *modelBase) -> bool
            {
                return (dynamic_cast<Model *>(modelBase) != nullptr);
            };


        auto & modelCreatorsByTypeName = PolyBase::ModelCreatorsByTypeName_();

        if (1 == modelCreatorsByTypeName.count(key))
        {
            throw std::logic_error(
                "Each Derived type must be registered only once.");
        }

        modelCreatorsByTypeName[key] =
            []() -> std::unique_ptr<ModelBase>
            {
                return std::make_unique<Model>();
            };
    }

private:
    using CreatorMap = std::map<std::string, CreatorFunction>;

    using ModelCheckersMap = std::map<std::string, CheckModelFunction>;
    using ModelCreatorsMap = std::map<std::string, CreateModelFunction>;

    // Construct On First Use Idiom
    static CreatorMap & CreatorsByTypeName_()
    {
        static CreatorMap map_;
        return map_;
    }

    static ModelCheckersMap & ModelCheckersByTypeName_()
    {
        static ModelCheckersMap map_;
        return map_;
    }

    static ModelCreatorsMap & ModelCreatorsByTypeName_()
    {
        static ModelCreatorsMap map_;
        return map_;
    }
};


} // end namespace poly


} // end namespace pex
