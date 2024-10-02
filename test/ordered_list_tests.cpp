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
        endpoint_(this, reorder, &ReorderObserver::OnReorder_),
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
