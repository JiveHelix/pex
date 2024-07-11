#pragma once


#include <ostream>
#include <fields/describe.h>


namespace pex
{


namespace poly
{


template<typename Json, typename T>
Json PolyUnstructure(const T &object)
{
    auto jsonValues = fields::Unstructure<Json>(object);
    jsonValues["type"] = T::fieldsTypeName;

    return jsonValues;
}


// Interface that will be implemented for us in PolyDerived.
template<typename Json_, typename Base>
class PolyBase
{
public:
    using Json = Json_;

    virtual ~PolyBase() {}

    virtual std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const = 0;

    virtual Json Unstructure() const = 0;
    virtual bool operator==(const Base &) const = 0;
    virtual std::string_view GetTypeName() const = 0;
    virtual std::shared_ptr<Base> Copy() const = 0;

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

    static constexpr auto polyTypeName = "PolyBase";

    using CreatorFunction =
        std::function<std::shared_ptr<Base> (const Json &jsonValues)>;

    template<typename Derived>
    static void RegisterDerived()
    {
        static_assert(
            fields::HasFieldsTypeName<Derived>,
            "Derived types must define a unique fieldsTypeName");

        auto key = std::string(Derived::fieldsTypeName);

        if (key.empty())
        {
            throw std::logic_error("fieldsTypeName is empty");
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
                    fields::StructureFromFields<Derived>(jsonValues));
            };
    }

private:
    using CreatorMap = std::map<std::string, CreatorFunction>;

    // Construct On First Use Idiom
    static CreatorMap & CreatorsByTypeName_()
    {
        static CreatorMap map_;
        return map_;
    }
};


} // end namespace poly


} // end namespace pex
