#include <thread>
#include <chrono>
#include <condition_variable>
#include <inttypes.h>

#ifdef WIN32
#define _CWIPC_UTIL_EXPORT __declspec(dllexport)
#else
#define _CWIPC_UTIL_EXPORT
#endif

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/logging.hpp"
#ifdef CWIPC_WITH_GUI
#include "window_util.hpp"

class cwipc_sink_window_impl : public cwipc_sink {
private:
    std::string m_title;
    window_util *m_window;
    cwipc_point *m_points;
    int m_npoints;
    float m_pointsize;
    float m_eye_angle;
    float m_eye_distance;
    float m_eye_height;
    bool m_left_mouse_pressed;
    bool m_right_mouse_pressed;
    float m_mouse_x;
    float m_mouse_y;
    std::string m_chars_wanted;
    char m_last_char;
    std::condition_variable m_last_char_cv;
    std::mutex m_last_char_mutex;
    bool m_render_skeleton;
    std::vector<cwipc_skeleton_joint> joints;

public:
    cwipc_sink_window_impl(const char *title)
    :   m_title(title),
        m_window(nullptr),
        m_points(nullptr),
        m_npoints(0),
        m_pointsize(0),
        m_eye_angle(0), m_eye_distance(1.8), m_eye_height(1),
        m_left_mouse_pressed(false),
        m_right_mouse_pressed(false),
        m_mouse_x(-1),
        m_mouse_y(-1),
        m_chars_wanted(""),
        m_last_char('\0'),
        m_render_skeleton(true)
    {
        m_window = new window_util(640, 480, m_title.c_str());
        m_window->on_left_mouse = std::bind(&cwipc_sink_window_impl::on_left_mouse, this, std::placeholders::_1);
        m_window->on_right_mouse = std::bind(&cwipc_sink_window_impl::on_right_mouse, this, std::placeholders::_1);
        m_window->on_mouse_move = std::bind(&cwipc_sink_window_impl::on_mouse_move, this, std::placeholders::_1, std::placeholders::_2);
        m_window->on_mouse_scroll = std::bind(&cwipc_sink_window_impl::on_mouse_scroll, this, std::placeholders::_1, std::placeholders::_2);
        m_window->on_character = std::bind(&cwipc_sink_window_impl::on_character, this, std::placeholders::_1);
    }

    ~cwipc_sink_window_impl() {
        delete m_window;
    }

    void free() {
        delete m_window;
        ::free(m_points);
        m_points = nullptr;
        m_npoints = 0;
    }

    bool feed(cwipc_pointcloud *pc, bool clear) {
        if (m_window == nullptr) {
            return false;
        }

        if (clear) {
            ::free(m_points);
            m_points = nullptr;
            m_npoints = 0;
        }

        if (pc) {
            if (pc->access_pcl_pointcloud() == nullptr) {
                cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_util", "cwipc_window.feed: NULL pointcloud");
                return false;
            }
            size_t bufferSize = pc->get_uncompressed_size();
            cwipc_point *newBuffer = (cwipc_point *)realloc(m_points, m_npoints*sizeof(cwipc_point)+bufferSize);

            if (newBuffer == nullptr) {
                return false;
            }

            m_points = newBuffer;
            newBuffer = m_points + m_npoints;
            int nNewPoints = pc->copy_uncompressed(newBuffer, bufferSize);
            m_npoints += nNewPoints;
            m_pointsize = pc->cellsize();

            if (m_render_skeleton) {
                cwipc_metadata* metadata = pc->access_metadata();

                if (metadata->count() > 0) {
                    get_skeleton(metadata);
                }
            }
        }
        
        m_window->prepare_gl(m_eye_distance*sin(m_eye_angle), m_eye_height, m_eye_distance*cos(m_eye_angle), m_pointsize);

        for (int i=0; i<m_npoints; i++) {
            glColor3ub(m_points[i].r, m_points[i].g, m_points[i].b);
            glVertex3f(m_points[i].x, m_points[i].y, m_points[i].z);
        }

        glEnd();

        //render skeleton
        if (m_render_skeleton && joints.size()>0) {
            render_skeleton();
        }

        //clean scene
        m_window->cleanup_gl();

        if (!*m_window) {
            return false;
        }

        return true;
    }

    void get_skeleton(cwipc_metadata* metadata) {
        bool found_skeleton = false;

        for (int i = 0; i < metadata->count(); i++) {
            void* ptr = metadata->pointer(i);

            if (metadata->name(i).find("skeleton") != std::string::npos) {
                if (!found_skeleton) {
                    cwipc_skeleton_collection* skl = (cwipc_skeleton_collection*)metadata->pointer(i);
                    int n_skeletons = skl->n_skeletons;

                    if (n_skeletons > 0 && skl->n_joints > 0) {
                        found_skeleton = true;
                        joints.clear();

                        for (int s = 0; s < skl->n_joints; s++) {
                            cwipc_skeleton_joint joint = skl->joints[s];
                            joints.push_back({ joint.confidence, joint.x, joint.y, joint.z, joint.q_w, joint.q_x, joint.q_y, joint.q_z });
                        }
                    }
                } else { // multiple cameras = multiple skeletons, so we need to fuse them
                    cwipc_skeleton_collection* new_skl = (cwipc_skeleton_collection*)metadata->pointer(i);
                    int n_joints = std::min((uint32_t)joints.size(), new_skl->n_joints);

                    for (int j = 0; j < n_joints; j++) {
                        cwipc_skeleton_joint old_joint = joints[j];
                        cwipc_skeleton_joint new_joint = new_skl->joints[j];

                        if (old_joint.confidence == new_joint.confidence) { // average positions
                            joints[j].x = (joints[j].x + new_joint.x) / 2;
                            joints[j].y = (joints[j].y + new_joint.y) / 2;
                            joints[j].z = (joints[j].z + new_joint.z) / 2;
                        } else if (old_joint.confidence < new_joint.confidence) { // use joint with higher confidence
                            joints[j] = { new_skl->joints[j].confidence, new_skl->joints[j].x, new_skl->joints[j].y, new_skl->joints[j].z, new_skl->joints[j].q_w, new_skl->joints[j].q_x, new_skl->joints[j].q_y, new_skl->joints[j].q_z };
                        }
                    }
                }
            }
        }
    }

    void render_skeleton() {
        glPointSize(6.0);
        glBegin(GL_POINTS);

        //render joints as points
        for (int i = 0; i < joints.size(); i++) {
            glColor3ub(255, 0, (255 / 3) * joints[i].confidence);
            glVertex3f(joints[i].x, joints[i].y, joints[i].z);
        }

        glEnd();

        //render bones:
        glLineWidth((GLfloat)5.0f);
        glBegin(GL_LINES);
        glColor3ub(0, 255, 255);

        //left leg
        boneline(joints[0], joints[18]);
        boneline(joints[18], joints[19]);
        boneline(joints[19], joints[20]);
        boneline(joints[20], joints[21]);

        //right leg
        boneline(joints[0], joints[22]);
        boneline(joints[22], joints[23]);
        boneline(joints[23], joints[24]);
        boneline(joints[24], joints[25]);

        //torso
        boneline(joints[0], joints[1]);
        boneline(joints[1], joints[2]);
        boneline(joints[2], joints[3]);

        //head
        boneline(joints[3], joints[26]);
        boneline(joints[26], joints[27]);
        boneline(joints[27], joints[28]);
        boneline(joints[28], joints[29]);
        boneline(joints[27], joints[30]);
        boneline(joints[30], joints[31]);

        //left arm
        boneline(joints[2], joints[4]);
        boneline(joints[4], joints[5]);
        boneline(joints[5], joints[6]);
        boneline(joints[6], joints[7]);
        boneline(joints[7], joints[8]);
        boneline(joints[8], joints[9]);
        boneline(joints[7], joints[10]);

        //right arm
        boneline(joints[2], joints[11]);
        boneline(joints[11], joints[12]);
        boneline(joints[12], joints[13]);
        boneline(joints[13], joints[14]);
        boneline(joints[14], joints[15]);
        boneline(joints[15], joints[16]);
        boneline(joints[14], joints[17]);

        glEnd();
    }

    void boneline(cwipc_skeleton_joint from, cwipc_skeleton_joint to) {
        glVertex3f(from.x, from.y, from.z);
        glVertex3f(to.x, to.y, to.z);
    }

    bool caption(const char *caption) {
        if (m_window == nullptr) {
            return false;
        }

        GLFWwindow *glfwWin = *m_window;
        std::string newTitle;

        if (caption) {
            newTitle = m_title + " - " + std::string(caption);
        } else {
            newTitle = m_title;
        }

        glfwSetWindowTitle(glfwWin, newTitle.c_str());
        return true;
    }

    char interact(const char *prompt, const char *responses, int32_t millis) {
        if (m_window == nullptr) {
            return '\0';
        }

        caption(prompt);
        m_chars_wanted = responses;
        std::chrono::system_clock::time_point until = std::chrono::system_clock::now();

        if (millis >= 0) {
            until += std::chrono::milliseconds(millis);
        } else {
            until += std::chrono::hours(24);
        }

        while (std::chrono::system_clock::now() < until && m_chars_wanted.find(m_last_char) == std::string::npos) {
            std::unique_lock<std::mutex> lock(m_last_char_mutex);
            m_last_char_cv.wait_for(lock, std::chrono::milliseconds(1), [this]{
                return this->m_last_char;
            });

            if (m_last_char) {
                break;
            }

            feed(nullptr, false);
        }

        char rv = m_last_char;
        m_last_char = '\0';

        if (rv == 'r') { // Toggle skeleton rendering
            m_render_skeleton = !m_render_skeleton;
            cwipc_log(CWIPC_LOG_LEVEL_TRACE, "cwipc_sink_window", "Skeleton rendering " + std::string(m_render_skeleton ? "enabled" : "disabled"));
        }

        return rv;
    }

private:
    void on_left_mouse(bool pressed) {
        m_left_mouse_pressed = pressed;
    }

    void on_right_mouse(bool pressed) {
        m_right_mouse_pressed = pressed;
    }

    void on_mouse_scroll(double deltax, double deltay) {
        m_eye_distance += deltay / 10;
    }

    void on_mouse_move(double x, double y) {
        if (m_left_mouse_pressed) {
            float delta_x = x - m_mouse_x;
            m_eye_angle += delta_x / 100;
        }

        if (m_right_mouse_pressed) {
            float delta_y = y - m_mouse_y;
            m_eye_height += delta_y / 100;
        }

        m_mouse_x = x;
        m_mouse_y = y;
    }

    void on_character(int c) {
        if (m_chars_wanted.find(c) == std::string::npos) {
            return;
        }

        m_last_char = c;
        m_last_char_cv.notify_one();
    }
};
#endif // CWIPC_WITH_GUI

cwipc_sink* cwipc_window(const char *title, char **errorMessage, uint64_t apiVersion) {
#ifdef CWIPC_WITH_GUI
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_window: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_sink* rv = new cwipc_sink_window_impl(title);
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_window", "unspecified error creating window sink");
    }
    cwipc_log_set_errorbuf(nullptr);
    return rv;
#else
	cwipc_log_set_errorbuf(errorMessage);
    cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_window", "cwipc was built without GUI support");
    cwipc_log_set_errorbuf(nullptr);

	return NULL;
#endif // CWIPC_WITH_GUI
}
