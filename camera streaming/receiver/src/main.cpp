///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

/*********************************************************************************
 ** This sample demonstrates how to capture and process the streaming video feed **
 ** provided by an application that uses the ZED SDK with streaming enabled.     **
 **********************************************************************************/

// Standard includes
#include <stdio.h>
#include <string.h>

// ZED include
#include <sl/Camera.hpp>

// OpenCV include (for display)
#include <opencv2/opencv.hpp>

// Using std and sl namespaces
using namespace std;
using namespace sl;

// Sample functions
void updateCameraSettings(char key, sl::Camera &zed);
void switchCameraSettings();
void switchViewMode();
void printHelp();
void print(std::string msg_prefix, ERROR_CODE err_code = ERROR_CODE::SUCCESS, std::string msg_suffix = "");

// Sample variables
VIDEO_SETTINGS camera_settings_ = VIDEO_SETTINGS::BRIGHTNESS;
VIEW view_mode = VIEW::LEFT;
string str_camera_settings = "BRIGHTNESS";
int step_camera_setting = 1;
bool led_on = true;

vector< string> split(const string& s, char seperator) {
    vector< string> output;
    string::size_type prev_pos = 0, pos = 0;

    while ((pos = s.find(seperator, pos)) != string::npos) {
        string substring(s.substr(prev_pos, pos - prev_pos));
        output.push_back(substring);
        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos));
    return output;
}

void setStreamParameter(InitParameters& init_p, string& argument) {
    vector< string> configStream = split(argument, ':');
    String ip(configStream.at(0).c_str());
    if (configStream.size() == 2) {
        init_p.input.setFromStream(ip, atoi(configStream.at(1).c_str()));
    } else init_p.input.setFromStream(ip);
}

int main(int argc, char **argv) {

#if 0
    auto streaming_devices = Camera::getStreamingDeviceList();
    int nb_streaming_zed = streaming_devices.size();

    print("Detect: " + std::to_string(nb_streaming_zed) + " ZED in streaming");
    if (nb_streaming_zed == 0) {
        print("No streaming ZED detected, have you take a look to the sample 'ZED Streaming Sender' ?");
        return 0;
    }

    for (auto& it : streaming_devices)
        cout << "* ZED: " << it.serial_number << ", IP: " << it.ip << ", port : " << it.port << ", bitrate : " << it.current_bitrate << "\n";
#endif

    Camera zed;
    // Set configuration parameters for the ZED
    InitParameters initParameters;
    initParameters.depth_mode = DEPTH_MODE::PERFORMANCE;
    initParameters.coordinate_system = COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP; // OpenGL's coordinate system is right_handed
    initParameters.sdk_verbose = true;

    string stream_params;
    if (argc > 1) {
        stream_params = string(argv[1]);
    } else {
        cout << "\nOpening the stream requires the IP of the sender\n";
        cout << "Usage : ./ZED_Streaming_Receiver IP:[port]\n";
        cout << "You can specify it now, then press ENTER, 'IP:[port]': ";
        cin >> stream_params;
    }

    setStreamParameter(initParameters, stream_params);


    // Open the camera
    ERROR_CODE err = zed.open(initParameters);
    if (err != ERROR_CODE::SUCCESS) {
        print("Opening ZED : ", err);
        zed.close();
        return 1; // Quit if an error occurred
    }

    // Print camera information
    auto camera_info = zed.getCameraInformation();
    printf("\n");
    printf("ZED Model                 : %s\n", toString(camera_info.camera_model).c_str());
    printf("ZED Serial Number         : %d\n", camera_info.serial_number);
    printf("ZED Camera Firmware       : %d-%d\n", camera_info.camera_firmware_version, camera_info.sensors_firmware_version);
    printf("ZED Camera Resolution     : %dx%d\n", (int) camera_info.camera_resolution.width, (int) camera_info.camera_resolution.height);
    printf("ZED Camera FPS            : %d\n", (int) zed.getInitParameters().camera_fps);

    // Print help in console
    printHelp();

    // Create a Mat to store images
    Mat zed_image;

    // Initialise camera setting
    switchCameraSettings();

    // Capture new images until 'q' is pressed
    char key = ' ';
    while (key != 'q') {
        // Check that grab() is successful
        err = zed.grab();
        if (err == ERROR_CODE::SUCCESS) {
            // Retrieve left image
            zed.retrieveImage(zed_image, view_mode);

            // Display image with OpenCV
            if (zed_image.getChannels() == 1)
                cv::imshow("VIEW", cv::Mat((int) zed_image.getHeight(), (int) zed_image.getWidth(), CV_8UC1, zed_image.getPtr<sl::uchar1>(sl::MEM::CPU)));
            else
                cv::imshow("VIEW", cv::Mat((int) zed_image.getHeight(), (int) zed_image.getWidth(), CV_8UC4, zed_image.getPtr<sl::uchar1>(sl::MEM::CPU)));

        } else {
            print("Error during capture : ", err);
            break;
        }

        key = cv::waitKey(3);
        // Change camera settings with keyboard
        updateCameraSettings(key, zed);
    }

    // Exit
    zed.close();
    return EXIT_SUCCESS;
}

/**
    This function updates camera settings
 **/
void updateCameraSettings(char key, sl::Camera &zed) {
    int current_value;

    // Keyboard shortcuts
    switch (key) {

            // Switch to the next view mode
        case 'v':
            switchViewMode();
            break;

            // Switch to the next camera parameter
        case 's':
            switchCameraSettings();
            current_value = zed.getCameraSettings(camera_settings_);
            break;

            // Increase camera settings value ('+' key)
        case '+':
            current_value = zed.getCameraSettings(camera_settings_);
            zed.setCameraSettings(camera_settings_, current_value + step_camera_setting);
            print(str_camera_settings + ": " + std::to_string(zed.getCameraSettings(camera_settings_)));
            break;

            // Decrease camera settings value ('-' key)
        case '-':
            current_value = zed.getCameraSettings(camera_settings_);
            current_value = current_value > 0 ? current_value - step_camera_setting : 0; // take care of the 'default' value parameter:  VIDEO_SETTINGS_VALUE_AUTO
            zed.setCameraSettings(camera_settings_, current_value);
            print(str_camera_settings + ": " + std::to_string(zed.getCameraSettings(camera_settings_)));
            break;

            //switch LED On :
        case 'l':
            led_on = !led_on;
            zed.setCameraSettings(sl::VIDEO_SETTINGS::LED_STATUS, led_on);
            break;

            // Reset to default parameters
        case 'r':
            print("Reset all settings to default");
            for (int s = (int) VIDEO_SETTINGS::BRIGHTNESS; s <= (int) VIDEO_SETTINGS::WHITEBALANCE_TEMPERATURE; s++)
                zed.setCameraSettings(static_cast<VIDEO_SETTINGS> (s), sl::VIDEO_SETTINGS_VALUE_AUTO);
            break;
    }
}

/**
    This function toggles between camera settings
 **/
void switchCameraSettings() {
    camera_settings_ = static_cast<VIDEO_SETTINGS> ((int) camera_settings_ + 1);

    // reset to 1st setting
    if (camera_settings_ == VIDEO_SETTINGS::LED_STATUS)
        camera_settings_ = VIDEO_SETTINGS::BRIGHTNESS;

    // select the right step
    step_camera_setting = (camera_settings_ == VIDEO_SETTINGS::WHITEBALANCE_TEMPERATURE) ? 100 : 1;

    // get the name of the selected SETTING
    str_camera_settings = string(sl::toString(camera_settings_).c_str());

    print("Switch to camera settings: ", ERROR_CODE::SUCCESS, str_camera_settings);
}

/**
    This function toggles between view mode
 **/
void switchViewMode() {
    view_mode = static_cast<VIEW> ((int) view_mode + 1);

    // reset to 1st setting
    if (view_mode == VIEW::DEPTH_RIGHT)
        view_mode = VIEW::LEFT;


    print("Switch to view mode: ", ERROR_CODE::SUCCESS, std::string(sl::toString(view_mode).c_str()));
}

/**
    This function displays help
 **/
void printHelp() {
    cout << "\n\nCamera controls hotkeys:\n";
    cout << "* Increase camera settings value:  '+'\n";
    cout << "* Decrease camera settings value:  '-'\n";
    cout << "* Toggle camera settings:          's'\n";
    cout << "* Toggle view mode:                'v'\n";
    cout << "* Toggle camera LED:               'l' (lower L)\n";
    cout << "* Reset all parameters:            'r'\n";
    cout << "* Exit :                           'q'\n\n";
}

void print(std::string msg_prefix, ERROR_CODE err_code, std::string msg_suffix) {
    cout << "[Sample]";
    if (err_code != ERROR_CODE::SUCCESS)
        cout << "[Error] ";
    else
        cout << " ";
    cout << msg_prefix << " ";
    if (err_code != ERROR_CODE::SUCCESS) {
        cout << " | " << toString(err_code) << " : ";
        cout << toVerbose(err_code);
    }
    if (!msg_suffix.empty())
        cout << " " << msg_suffix;
    cout << endl;
}

void parseArgs(int argc, char **argv, sl::InitParameters& param) {
    if (argc > 1 && string(argv[1]).find(".svo") != string::npos) {
        // SVO input mode not available in camera control
        std::cout << "SVO Input mode is not available for camera control sample" << std::endl;
    } else if (argc > 1 && string(argv[1]).find(".svo") == string::npos) {
        string arg = string(argv[1]);
        unsigned int a, b, c, d, port;
        if (sscanf(arg.c_str(), "%u.%u.%u.%u:%d", &a, &b, &c, &d, &port) == 5) {
            // Stream input mode - IP + port
            string ip_adress = to_string(a) + "." + to_string(b) + "." + to_string(c) + "." + to_string(d);
            param.input.setFromStream(sl::String(ip_adress.c_str()), port);
            cout << "[Sample] Using Stream input, IP : " << ip_adress << ", port : " << port << endl;
        } else if (sscanf(arg.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            // Stream input mode - IP only
            param.input.setFromStream(sl::String(argv[1]));
            cout << "[Sample] Using Stream input, IP : " << argv[1] << endl;
        } else if (arg.find("HD2K") != string::npos) {
            param.camera_resolution = sl::RESOLUTION::HD2K;
            cout << "[Sample] Using Camera in resolution HD2K" << endl;
        } else if (arg.find("HD1080") != string::npos) {
            param.camera_resolution = sl::RESOLUTION::HD1080;
            cout << "[Sample] Using Camera in resolution HD1080" << endl;
        } else if (arg.find("HD720") != string::npos) {
            param.camera_resolution = sl::RESOLUTION::HD720;
            cout << "[Sample] Using Camera in resolution HD720" << endl;
        } else if (arg.find("VGA") != string::npos) {
            param.camera_resolution = sl::RESOLUTION::VGA;
            cout << "[Sample] Using Camera in resolution VGA" << endl;
        }
    } else {
        // Default
    }
}
