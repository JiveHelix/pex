#include <catch2/catch.hpp>
#include <pex/ordered_list.h>
#include <pex/endpoint.h>


TEST_CASE("OrderedList iterates in order", "[OrderedList]")
{
    using List = pex::List<int, 0>;
    using OrderedListGroup = pex::OrderedListGroup<List>;

    using Model = typename OrderedListGroup::Model;
    using Control = typename OrderedListGroup::Control;

    Model model;
    Control control(model);

    for (int i = 0; i < 4; ++i)
    {
        model.Append(i);
    }

    REQUIRE(model.count.Get() == 4);
    REQUIRE(control.count.Get() == 4);

    REQUIRE(control[0].Get() == 0);
    REQUIRE(control[1].Get() == 1);

    model.indices.Set({1, 0, 2, 3});

    REQUIRE(control[0].Get() == 1);
    REQUIRE(control[1].Get() == 0);

    auto plain = control.Get();

    auto begin = std::begin(plain);

    REQUIRE(*begin == 1);
    REQUIRE(*(++begin) == 0);
}


class ReorderObserver
{
public:
    ReorderObserver(pex::control::Signal<> reorder)
        :
        endpoint_(
            PEX_THIS("ReorderObserver"),
            reorder,
            &ReorderObserver::OnReorder_),
        count(0)
    {

    }

    void OnReorder_()
    {
        ++this->count;
    }

    pex::Endpoint<ReorderObserver, pex::control::Signal<>> endpoint_;
    size_t count;
};


TEST_CASE("OrderedList signals when order changes", "[OrderedList]")
{
    using List = pex::List<int, 0>;
    using OrderedListGroup = pex::OrderedListGroup<List>;

    using Model = typename OrderedListGroup::Model;
    using Control = typename OrderedListGroup::Control;

    Model model;
    Control control(model);

    for (int i = 0; i < 4; ++i)
    {
        model.Append(i);
    }

    ReorderObserver observer(model.reorder);

    model.indices.Set({1, 0, 2, 3});

    REQUIRE(observer.count == 1);
}


TEST_CASE("OrderedList can delete selected", "[OrderedList]")
{
    using List = pex::List<int, 0>;
    using OrderedListGroup = pex::OrderedListGroup<List>;

    using Model = typename OrderedListGroup::Model;
    using Control = typename OrderedListGroup::Control;

    Model model;
    Control control(model);

    for (int i = 0; i < 4; ++i)
    {
        model.Append(i);
    }

    ReorderObserver observer(model.reorder);

    model.indices.Set({3, 2, 1, 0});


    REQUIRE(observer.count == 1);

    control.selected.Set(2);

    REQUIRE(control[1].Get() == 2);

    control.EraseSelected();

    // We erased the element at storage index 2, causing the remaining elements
    // to shift left.
    REQUIRE(control[1].Get() == 1);
}


template<typename ListControl>
class TestListObserver
{
public:
    using ListEndpoint =
        pex::Endpoint<TestListObserver, ListControl>;

    using ListType = typename ListControl::Type;

    TestListObserver(ListControl listControl)
        :
        endpoint_(
            PEX_THIS("TestListObserver"),
            listControl,
            &TestListObserver::OnList_),

        observedList_(listControl.Get()),
        notificationCount_()
    {

    }

    void OnList_(const ListType &listType)
    {
        this->observedList_ = listType;
        ++this->notificationCount_;
    }

    size_t GetNotificationCount() const
    {
        return this->notificationCount_;
    }

    const ListType & GetList() const
    {
        return this->observedList_;
    }

    bool operator==(const ListType &other) const
    {
        if (other.size() != this->observedList_.size())
        {
            return false;
        }

        for (size_t i = 0; i < other.size(); ++i)
        {
            if (other[i] != this->observedList_[i])
            {
                return false;
            }
        }

        return true;
    }

private:
    ListEndpoint endpoint_;
    ListType observedList_;
    size_t notificationCount_;
};


TEST_CASE("OrderedList Erase by index", "[OrderedList]")
{
    using List = pex::List<int, 0>;
    using OrderedListGroup = pex::OrderedListGroup<List>;

    using Model = typename OrderedListGroup::Model;
    using Control = typename OrderedListGroup::Control;

    Model model;
    PEX_ROOT(model);
    Control control(model);

    for (int i = 0; i < 4; ++i)
    {
        model.Append(i);
    }

    TestListObserver observer(control);

    REQUIRE(model.size() == 4);

    model.Erase(2);

    REQUIRE(model.size() == 3);

    auto editedList = model.Get();

    REQUIRE(observer == editedList);

    // We erased the element at storage index 2, causing the remaining elements
    // to shift left.
    REQUIRE(control[2].Get() == 3);
    REQUIRE(editedList.at(2) == 3);
}


TEST_CASE("Reordered OrderedList Erase by index", "[OrderedList]")
{
    using List = pex::List<int, 0>;
    using OrderedListGroup = pex::OrderedListGroup<List>;

    using Model = typename OrderedListGroup::Model;
    using Control = typename OrderedListGroup::Control;

    Model model;
    PEX_ROOT(model);
    Control control(model);

    for (int i = 0; i < 4; ++i)
    {
        model.Append(i);
    }

    TestListObserver observer(control);

    REQUIRE(model.size() == 4);

    model.MoveToTop(3);

    REQUIRE(model.indices.at(0) == 3);

    std::vector<int> expectedSort{{3, 0, 1, 2}};

    REQUIRE(observer == expectedSort);

    // Erase the item at storage index 2
    model.Erase(2);

    REQUIRE(model.size() == 3);

    auto editedList = model.Get();

    REQUIRE(observer == editedList);

    // The items index in control are ordered.
    REQUIRE(control[2].Get() == 1);
    REQUIRE(editedList.at(2) == 1);

    model.Append(42);
    REQUIRE(control.at(3).Get() == 42);
    REQUIRE(observer.GetList().at(3) == 42);
}
