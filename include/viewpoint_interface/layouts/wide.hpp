#ifndef __LAYOUT_WIDE_HPP__
#define __LAYOUT_WIDE_HPP__

#include "viewpoint_interface/layout.hpp"

namespace viewpoint_interface
{

struct WideParams
{
    uint primary_display = 1;
};

class WideLayout : public Layout
{
public:
    WideLayout(DisplayManager &displays, WideParams params=WideParams()) : 
            Layout(LayoutType::WIDE, displays), parameters_(params) 
    {
        addPrimaryDisplayByIx(parameters_.primary_display);
        frame_mode_ = FrameMode::WORLD_FRAME;
    }

    virtual void displayLayoutParams() override
    {
        drawDisplaysList();

        drawDraggableRing();

        drawDisplaySelector(0, "Main Display", LayoutDisplayRole::Primary);
    }

    virtual void draw() override
    {
        handleImageResponse();

        std::map<std::string, bool> states;
        states["Robot"] = !clutching_;
        states["Suction"] = grabbing_;
        displayStateValues(states);

        displayPrimaryWindows();

        std::vector<uchar> &prim_data = displays_.getDisplayDataById(primary_displays_.at(0));
        const DisplayInfo &prim_info(displays_.getDisplayInfoById(primary_displays_.at(0)));
        addImageRequestToQueue(DisplayImageRequest{prim_info.dimensions.width, prim_info.dimensions.height,
                prim_data, (uint)0, LayoutDisplayRole::Primary});           
    }

    virtual void handleImageResponse() override
    {
        for (int i = 0; i < image_response_queue_.size(); i++) {
            DisplayImageResponse &response(image_response_queue_.at(i));
            prim_img_ids_[response.index] = response.id;
        }
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