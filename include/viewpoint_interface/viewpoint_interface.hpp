#ifndef __VIEWPOINT_INTERFACE_HPP__
#define __VIEWPOINT_INTERFACE_HPP__

#include <string>
#include <vector>

#include "ros/ros.h"

// TEST
#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtx/string_cast.hpp>

#include "viewpoint_interface/shader.hpp"
#include "viewpoint_interface/scene_camera.hpp"


namespace viewpoint_interface
{
    struct Socket
    {
        uint const PORT = 8080;
        uint static const DATA_SIZE = 2048;

        int sock;
        sockaddr_in addr;
        socklen_t len;
        char buffer[DATA_SIZE];
    };

    struct AppParams
    {
        uint loop_rate = 60;

        // Starting window dimensions - 0 indicates fullscreen
        uint const WINDOW_WIDTH = 0;
        uint const WINDOW_HEIGHT = 0;

        uint pip_width = WINDOW_WIDTH * 0.25;
        uint pip_height = WINDOW_HEIGHT * 0.25;

        uint def_disp_height = 1024;
        uint def_disp_width = 1024;
        uint def_disp_channels = 3;
        std::string cam_config_file = "resources/config/cam_config.json";
        const std::string CONTR_NAME = "vive_controller";
    };

    struct Image
    {
        uint width, height, channels, size, id;
        std::vector<uchar> data;

        Image() : width(0), height(0), channels(0), size(0) {}
        Image(uint w, uint h, uint c) : width(w), height(h), channels(c)
        {
            size = w * h * c;
            data = std::vector<uchar>(size);
        }

        void copy_data(const cv::Mat &mat)
        {
            std::vector<uchar> new_data;
            if (mat.isContinuous()) {
                new_data.assign(mat.data, mat.data + mat.total()*mat.channels());
            } else {
                for (int i = 0; i < mat.rows; ++i) {
                    new_data.insert(new_data.end(), mat.ptr<uchar>(i), mat.ptr<uchar>(i)+mat.cols*mat.channels());
                }
            }
            data = new_data;
        }

        void generateEmptyTexture(uint tex_num);

        void resize(uint w, uint h, uint c)  
        {
            width = w; height = h; channels = c;
            size = width * height * channels;

            data.reserve(size);
        }
    };

    struct Display
    {
        std::string name, topic_name, display_name;
        Image image;

        Display() : name(""), topic_name(""), display_name("") {}
        Display(std::string n, std::string t, std::string d, uint w, uint h, uint c) : name(n), topic_name(t), 
                display_name(d), image(Image(w, h, c)) {}
    };

    struct Mesh
    {
        uint VBO, VAO, EBO, tex;
    };

    struct Switch
    {
        enum class Type 
        {
            HOLD, // Turn on while holding button
            SINGLE, // Toggle with each press
            DOUBLE // Double-press to toggle
        };

        Switch(bool state=false, Type type=Type::HOLD) : m_state(state),  m_type(type),
                m_cur_signal(false), m_prev_signal(false), m_flipping(false), m_unconfirmed(false) {}

        bool is_on() { return m_state; }
        bool is_flipping() { return m_flipping; }
        std::string to_str() { return (m_state ? "on" : "off"); }

        void turn_on() { m_state = true; m_flipping = true; m_unconfirmed = true; }
        void turn_off() { m_state = false; m_flipping = true; m_unconfirmed = true; }
        void flip() { m_state = !m_state; m_flipping = true; m_unconfirmed = true; }
        void set_state(bool state) { m_state = state; }
        bool confirm_flip() {
            if (m_unconfirmed) {
                m_unconfirmed = false;
                return true;
            }

            return false;
        }
        bool confirm_flip_on() {
            return confirm_flip() && is_on();
        }
        bool confirm_flip_off() {
            return confirm_flip() && !is_on();    
        }

        // TODO: Consider breaking this out into button object
        bool button_pressed() { return m_cur_signal && !m_prev_signal; }
        bool button_depressed() { return !m_cur_signal && m_prev_signal; }
        void set_signal(bool signal) 
        { 
            m_prev_signal = m_cur_signal;
            m_cur_signal = signal;

            m_flipping = false;
            switch(m_type)
            {
                case Type::HOLD:
                {
                    m_state = m_cur_signal;
                    if (button_pressed() || button_depressed()) {
                        flip();
                    }
                } break;
                case Type::SINGLE:
                {
                    if (button_pressed()) {
                        flip();
                    }
                } break;
                case Type::DOUBLE:
                {
                    // TODO:
                    // When first press happens:
                    // - Set sentinel 'wait' variable
                    // - Start timer (1.5sec?)
                    // - Check time:
                    //      * Timer expired--set wait to false, end
                    // - If button pressed again, flip switch
                } break;
            }
        }

        void operator =(const bool val) { set_signal(val); }

    private:
        bool m_state;
        bool m_cur_signal;
        bool m_prev_signal;
        Type m_type;
        bool m_flipping; // Is switch flipping this cycle?
        bool m_unconfirmed;
    };

    struct Input
    {
        glm::vec3 manual_offset;
        bool initialized;
        Switch manual_adj;
        Switch clutching;
        glm::vec3 clutch_offset;
        cv_bridge::CvImagePtr cur_img;
        bool dyn_valid, stat_valid;


        Input()
        {
            manual_offset = glm::vec3();
            initialized = false;
            manual_adj = Switch(false, Switch::Type::HOLD);
            clutching = Switch(false, Switch::Type::SINGLE);
            clutch_offset = glm::vec3();
            dyn_valid, stat_valid = false;
        }

        std::string to_str(bool show_euler=false)
        {
            std::string content;
            content += "Manual Adj: \t" + manual_adj.to_str();
            content +=  "\t" + glm::to_string(manual_offset) + "\n";
            content += "Clutch: " + clutching.to_str() + "\n";

            return content;
        }
    };

    class App
    {
    public:
        static const int FRAME_X = 0;
        static const int FRAME_Y = 0;
        static constexpr float WIDTH_FAC = 1.0f;
        static constexpr float HEIGHT_FAC = 1.0f;

        App(AppParams params = AppParams()) : app_params(params)
        {
            pip_enabled = clutch_mode = false;
            active_display = pip_display = 0;
        }

        int run(int argc, char *argv[]);

        static void transformFramebufferDims(int *x, int *y, int *width, int *height);

    private:
        // General items
        AppParams app_params;
        Input input;
        Socket sock;
        std::map<uint, Display> disp_info;
        uint active_display;
        bool pip_enabled; // pip = Picture-in-picture
        uint pip_tex;
        uint pip_display;
        bool clutch_mode;

        // ROS
        std::vector<ros::Subscriber> cam_subs;

        // GUI
        GLFWwindow* window;
        GLFWmonitor *monitor;
        ImGuiIO io;
        Image out_img;
        Image pip_img;


        // General program flow
        bool initialize(int argc, char *argv[]);
        bool parseCameraFile();
        bool initializeSocket();
        void initializeROS(int argc, char *argv[]);
        bool initializeGlfw();
        void initializeImGui();
        void shutdownApp();

        // Input handling
        void parseControllerInput(std::string data);
        void handleRobotControl();

        void cameraImageCallback(const sensor_msgs::ImageConstPtr& msg, int index);
        static void keyCallbackForwarding(GLFWwindow* window, int key, int scancode, int action, int mods);
        void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        void updateOutputImage();
        void updatePipImage();
        
        // Dear ImGui
        void buildMenu(const char *title, void (App::*build_func)(void), ImGuiWindowFlags window_flags = 0);
        void buildDisplaySelectors();
        void buildPiPWindow();
    };

} // viewpoint_interface

void printText(std::string text="", int newlines=1, bool flush=false);
uint getNextIndex(uint ix, uint size);
uint getPreviousIndex(uint ix, uint size);
void mismatchDisplays(uint &disp1, uint &disp2, uint size);
void nextDisplay(uint &disp1, uint &disp2, uint size, bool bump=true);
void previousDisplay(uint &disp1, uint &disp2, uint size, bool bump=true);

void glfwErrorCallback(int code, const char* description);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

std::string getData(viewpoint_interface::Socket &sock);

viewpoint_interface::Mesh generateSquare();


#endif // __VIEWPOINT_INTERFACE_HPP__
