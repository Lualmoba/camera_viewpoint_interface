#ifndef __LAYOUT_WIDE_HPP__
#define __LAYOUT_WIDE_HPP__

#include "viewpoint_interface/layout.hpp"

namespace viewpoint_interface
{

struct WideParams
{
    uint primary_display = 1;
};

class WideLayout final : public Layout
{
public:
    WideLayout(DisplayManager &displays, WideParams params=WideParams()) : 
            Layout(LayoutType::WIDE, displays), parameters_(params) 
    {
        addDisplayByIxAndRole(parameters_.primary_display, LayoutDisplayRole::Primary);
    }

    virtual void displayLayoutParams() override
    {
        drawDisplaysList();
        drawDraggableRing();
        drawDisplaySelector(0, "Main Display", LayoutDisplayRole::Primary);
    }

    virtual void draw() override
    {
        addLayoutComponent(LayoutComponent::Type::Primary);
        drawLayoutComponents();

        std::map<std::string, bool> states;
        states["Robot"] = !clutching_;
        states["Suction"] = grabbing_;
        displayStateValues(states);
    }

    virtual void handleControllerInput(std::string input) override
    {
        LayoutCommand command(translateControllerInputToCommand(input));

        switch(command)
        {
            default:
            {}  break;
        }
    }

private:
    WideParams parameters_;
};

} // viewpoint_interface

#endif //__LAYOUT_WIDE_HPP__