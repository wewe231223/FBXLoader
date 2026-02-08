#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

#include <glad/glad.h>

#ifdef USE_GLEW
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Common.h"
#include "Timer.h"
#include "Input.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Model.h"
#include "Renderer.h"

#include "UfbxAssetLoader.h"
#include "MeshHierarchyBuilder.h"
#include "FontAtlas.h"
#include "TextRenderer.h"


namespace fs = std::filesystem;

namespace
{
    constexpr int WindowWidth = 1280;
    constexpr int WindowHeight = 720;

    asset::Model CreateAxisModel()
    {
        // --- Parameters (tweak as you like) ---
        constexpr float AxisLength = 2000.0f;  // axis line extends to +/- this value
        constexpr float TickInterval = 1.0f;   // tick spacing
        constexpr float TickSize = 0.05f;  // half-length of each tick segment

        // We'll build a GL_LINES model:
        // - 3 main axis lines (+/-X, +/-Y, +/-Z)
        // - tick marks every 1.0 unit (excluding origin) on each axis
        std::vector<asset::Vertex> v;
        std::vector<uint32_t> i;

        v.reserve(6 + static_cast<size_t>((AxisLength * 2.0f / TickInterval) * 3.0f * 2.0f));
        i.reserve(6 + static_cast<size_t>((AxisLength * 2.0f / TickInterval) * 3.0f * 2.0f));

        auto make = [](const glm::vec3& p, const glm::vec4& c) -> asset::Vertex
            {
                asset::Vertex vx{};
                vx.Position = p;
                vx.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vx.TexCoord = glm::vec2(0.0f);
                vx.Color = c;
                return vx;
            };

        auto addLine = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec4& c0, const glm::vec4& c1)
            {
                const uint32_t base = static_cast<uint32_t>(v.size());
                v.push_back(make(a, c0));
                v.push_back(make(b, c1));
                i.push_back(base + 0);
                i.push_back(base + 1);
            };

        auto addTick = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec4& color)
            {
                const uint32_t base = static_cast<uint32_t>(v.size());
                v.push_back(make(a, color));
                v.push_back(make(b, color));
                i.push_back(base + 0);
                i.push_back(base + 1);
            };

        // --- Main axis lines ---
        // Colors:
        //  +X red, -X dark red
        //  +Y green, -Y dark green
        //  +Z blue, -Z dark blue
        addLine(glm::vec3(-AxisLength, 0.0f, 0.0f), glm::vec3(AxisLength, 0.0f, 0.0f),
            glm::vec4(0.5f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // X

        addLine(glm::vec3(0.0f, -AxisLength, 0.0f), glm::vec3(0.0f, AxisLength, 0.0f),
            glm::vec4(0.0f, 0.5f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); // Y

        addLine(glm::vec3(0.0f, 0.0f, -AxisLength), glm::vec3(0.0f, 0.0f, AxisLength),
            glm::vec4(0.0f, 0.0f, 0.5f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)); // Z

        // --- Tick marks every 1.0 unit (skip origin) ---
        // X-axis ticks: short segments along Y at z=0
        for (float x = -AxisLength; x <= AxisLength + 0.0001f; x += TickInterval)
        {
            if (std::abs(x) < 0.0001f)
            {
                continue;
            }

            addTick(glm::vec3(x, -TickSize, 0.0f),
                glm::vec3(x, TickSize, 0.0f),
                glm::vec4(0.7f, 0.0f, 0.0f, 1.0f));
        }

        // Y-axis ticks: short segments along X at z=0
        for (float y = -AxisLength; y <= AxisLength + 0.0001f; y += TickInterval)
        {
            if (std::abs(y) < 0.0001f)
            {
                continue;
            }

            addTick(glm::vec3(-TickSize, y, 0.0f),
                glm::vec3(TickSize, y, 0.0f),
                glm::vec4(0.0f, 0.7f, 0.0f, 1.0f));
        }

        // Z-axis ticks: short segments along Y at x=0
        for (float z = -AxisLength; z <= AxisLength + 0.0001f; z += TickInterval)
        {
            if (std::abs(z) < 0.0001f)
            {
                continue;
            }

            addTick(glm::vec3(0.0f, -TickSize, z),
                glm::vec3(0.0f, TickSize, z),
                glm::vec4(0.0f, 0.0f, 0.7f, 1.0f));
        }

        asset::Model m;
        m.Create(v, i, GL_LINES);
        return m;
    }

    asset::Model CreateTexturedCube()
    {
        // A cube with positions, normals, UVs.
        // 24 vertices (unique per-face) + 36 indices.
        std::vector<asset::Vertex> v;
        v.reserve(24);

        const glm::vec4 white(1.0f);

        auto addFace = [&](glm::vec3 n,
                           glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) // CCW
        {
            asset::Vertex a{};
            a.Position = p0; a.Normal = n; a.TexCoord = {0.0f, 0.0f}; a.Color = white;
            asset::Vertex b{};
            b.Position = p1; b.Normal = n; b.TexCoord = {1.0f, 0.0f}; b.Color = white;
            asset::Vertex c{};
            c.Position = p2; c.Normal = n; c.TexCoord = {1.0f, 1.0f}; c.Color = white;
            asset::Vertex d{};
            d.Position = p3; d.Normal = n; d.TexCoord = {0.0f, 1.0f}; d.Color = white;

            v.push_back(a);
            v.push_back(b);
            v.push_back(c);
            v.push_back(d);
        };

        const float s = 0.5f;

        // +Z
        addFace({0,0,1}, {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s});
        // -Z
        addFace({0,0,-1}, { s,-s,-s}, {-s,-s,-s}, {-s, s,-s}, { s, s,-s});
        // +X
        addFace({1,0,0}, { s,-s, s}, { s,-s,-s}, { s, s,-s}, { s, s, s});
        // -X
        addFace({-1,0,0}, {-s,-s,-s}, {-s,-s, s}, {-s, s, s}, {-s, s,-s});
        // +Y
        addFace({0,1,0}, {-s, s, s}, { s, s, s}, { s, s,-s}, {-s, s,-s});
        // -Y
        addFace({0,-1,0}, {-s,-s,-s}, { s,-s,-s}, { s,-s, s}, {-s,-s, s});

        std::vector<uint32_t> idx;
        idx.reserve(36);
        for (uint32_t f = 0; f < 6; ++f)
        {
            const uint32_t base = f * 4;
            idx.push_back(base + 0);
            idx.push_back(base + 1);
            idx.push_back(base + 2);

            idx.push_back(base + 0);
            idx.push_back(base + 2);
            idx.push_back(base + 3);
        }

        asset::Model m;
        m.Create(v, idx, GL_TRIANGLES);
        return m;
    }

    void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        (void)window;
        glViewport(0, 0, width, height);
    }



    asset::ModelResult OnFileDropped(const std::string& path, std::vector<std::pair<asset::Model,const asset::ModelNode*>>& models)
    {
		std::filesystem::path fsPath(path);

        models.clear();

        asset::UfbxAssetLoader loader{ asset::GraphicsAPI::OpenGL };
        asset::ModelResult res{}; 
		asset::MeshHierarchyBuilder builder{ res };

		asset::ISceneNodeVisitor* visitors[] = { &builder };

        if (fsPath.extension() != ".fbx" and fsPath.extension() != ".FBX") {
            std::cout << "Wrong file type - " << path << "\nOnly.fbx files are supported.\n";
            return res; 
        }


        loader.LoadAndTraverse(path, { visitors });

        res.ForEachDFS([&models](const asset::ModelNode& node)
            {
                if (node.Vertices().Empty()) {
                    return; 
                }


                asset::Model model;
                model.Create(
                    node.Vertices().Data<asset::Vertex>(), 
                    node.Indices().Data<uint32_t>(), 
                    GL_TRIANGLES
                );
				models.emplace_back(std::move(model), &node);
            }
        );

        std::cout << "[Drop] " << path << "\n";
        return res; 
    }


    glm::mat4 ComputeWorldMatrix(const asset::ModelNode& node)
    {
        glm::mat4 world{ 1.0f };

        std::vector<const asset::ModelNode*> chain{ node.GetChildChain() }; 

        // root부터 내려오면서 누적
        for (std::size_t i = 0; i < chain.size(); ++i)
        {
            const asset::ModelNode* n = chain[i];
            world = world * n->GetNodeToParent();
			world = world * n->GetGeometryToNode();
        }

        return world;
    }

    static std::string FindSystemFontTtf()
    {
        namespace fs = std::filesystem;

#if defined(_WIN32)
        const std::vector<std::string> candidates = {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
        };
#elif defined(__APPLE__)
        const std::vector<std::string> candidates = {
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/System/Library/Fonts/Supplemental/Helvetica.ttf",
            "/System/Library/Fonts/SFNS.ttf",
        };
#else // Linux
        const std::vector<std::string> candidates = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        };
#endif

        for (const auto& p : candidates)
        {
            if (fs::exists(p))
            {
                return p;
            }
        }
        return {};
    }

    static void DrawAxisTickLabels(asset::TextRenderer& tr,
        const asset::FontAtlas& font,
        asset::Shader& textShader,
        const asset::Camera& camera,
        const glm::mat4& view,
        const glm::mat4& proj,
        int fbw,
        int fbh)
    {
        if (font.TextureId() == 0)
        {
            return;
        }

        constexpr float AxisLength = 15.0f;
        constexpr float TickInterval = 1.0f;

        // 화면에서 보이는 글자 크기(픽셀 단위)
        constexpr float LabelPx = 15.0f;

        // 텍스트가 눈금과 겹치지 않도록 약간 띄우기(월드 단위)
        constexpr float Offset = 0.10f;

        auto drawOne = [&](const std::string& s, const glm::vec3& pos, const glm::vec4& color)
            {
                tr.DrawTextBillboard(font, textShader, s, pos,
                    camera, view, proj, fbw, fbh,
                    LabelPx, color,
                    /*depthTest*/ true);
            };

        // X축: (x,0,0) 지점에 "1m" 등
        for (float x = -AxisLength; x <= AxisLength + 0.0001f; x += TickInterval)
        {
            if (std::abs(x) < 0.0001f) continue;

            const int v = static_cast<int>(std::round(x));
            drawOne(std::to_string(v) + "m", glm::vec3(x, Offset, 0.0f), glm::vec4(1, 1, 1, 1));
        }

        // Y축
        for (float y = -AxisLength; y <= AxisLength + 0.0001f; y += TickInterval)
        {
            if (std::abs(y) < 0.0001f) continue;

            const int v = static_cast<int>(std::round(y));
            drawOne(std::to_string(v) + "m", glm::vec3(Offset, y, 0.0f), glm::vec4(1, 1, 1, 1));
        }

        // Z축
        for (float z = -AxisLength; z <= AxisLength + 0.0001f; z += TickInterval)
        {
            if (std::abs(z) < 0.0001f) continue;

            const int v = static_cast<int>(std::round(z));
            drawOne(std::to_string(v) + "m", glm::vec3(0.0f, Offset, z), glm::vec4(1, 1, 1, 1));
        }
    }

}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, "OpenGL Model Viewer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return 1;
    }

#ifdef USE_GLEW
    glewExperimental = GL_TRUE;
    const GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewErr) << "\n";
        glfwTerminate();
        return 1;
    }
#endif

    asset::Renderer renderer;
    renderer.Initialize();
    renderer.Resize(WindowWidth, WindowHeight);

    asset::Timer timer;

    asset::Input input(window);
    asset::Input::InstallCallbacks(window, &input);

    asset::Camera camera;
    camera.SetAspect(static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight));
    camera.SetTarget(glm::vec3(0.0f));
    camera.SetDistance(5.0f);
    camera.SetYawPitch(glm::radians(45.0f), glm::radians(-20.0f));

    asset::Shader lit;
    asset::Shader axis;

    const fs::path shaderDir = fs::current_path() / "shaders";
    if (!lit.LoadFromFiles((shaderDir / "lit.vert").string(), (shaderDir / "lit.frag").string()))
    {
        std::cerr << "Failed to load lit shader\n";
        return 1;
    }
    if (!axis.LoadFromFiles((shaderDir / "axis.vert").string(), (shaderDir / "axis.frag").string()))
    {
        std::cerr << "Failed to load axis shader\n";
        return 1;
    }

    asset::FontAtlas font;
    asset::TextRenderer textRenderer;
    asset::Shader textShader;

    const std::string fontPath = FindSystemFontTtf();
    if (!fontPath.empty())
    {
        // bakePixelHeight는 아틀라스에 구워 넣는 기준 픽셀 높이
        if (!font.LoadFromTtfFile(fontPath, 32.0f, 512, 512, 32, 96))
        {
            std::cerr << "Failed to load font atlas: " << fontPath << "\n";
        }
    }
    else
    {
        std::cerr << "No system font found. Provide a .ttf.\n";
    }


    std::cout << "Font path: " << fontPath << "\n";

    const bool fontOk = (!fontPath.empty()) &&
        font.LoadFromTtfFile(fontPath, 32.0f, 512, 512, 32, 96);

    std::cout << "Font loaded: " << (fontOk ? "OK" : "FAILED")
        << " tex=" << font.TextureId()
        << " atlas=" << font.AtlasWidth() << "x" << font.AtlasHeight()
        << " bake=" << font.BakePixelHeight()
        << "\n";


    // 텍스트 셰이더 로드
    if (!textShader.LoadFromFiles((shaderDir / "text.vert").string(),
        (shaderDir / "text.frag").string()))
    {
        std::cerr << "Failed to load text shader\n";
    }

    textRenderer.Initialize();


    asset::Texture2D checker;
    const fs::path assetPath = fs::current_path() / "assets" / "checker.png";
    const bool hasTex = checker.LoadFromFile(assetPath.string(), true);

    asset::Model axisModel = CreateAxisModel();
    asset::Model cubeModel = CreateTexturedCube();

	std::vector<std::pair<asset::Model,const asset::ModelNode*>> models;
    asset::ModelResult res; 
    


    glm::vec3 lightPos(2.0f, 1.5f, 2.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window))
    {
        timer.Tick();
        input.NewFrame();
        input.Poll();

        {
            const auto dropped = input.ConsumeDroppedFiles();
            for (const auto& p : dropped)
            {
                res = OnFileDropped(p, models);   
            }

            if (dropped.size() != 0) {
                const auto& b = models[0].first.GetBounds(); 
				auto r = models[0].first.GetBoundingSphereRadius();

                camera.FrameBounds(b.Center(), r, 1.25f);
            }
        }



        if (input.KeyPressed(GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // Camera: orbit with LMB drag, zoom with scroll.
        if (input.MouseDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            const glm::vec2 d = input.MouseDelta();
            const float rotSpeed = 0.006f;
            camera.AddYawPitch(-d.x * rotSpeed, -d.y * rotSpeed);
        }
        {
            const float scroll = input.ScrollDelta();
            if (scroll != 0.0f)
            {
                camera.AddZoom(scroll * 0.5f);
            }
        }

        // Light move (world axis)
        const float moveSpeed = 2.5f;
        const float dt = timer.DeltaSeconds();
        if (input.KeyDown(GLFW_KEY_W)) { lightPos.z += moveSpeed * dt; }
        if (input.KeyDown(GLFW_KEY_S)) { lightPos.z -= moveSpeed * dt; }
        if (input.KeyDown(GLFW_KEY_A)) { lightPos.x -= moveSpeed * dt; }
        if (input.KeyDown(GLFW_KEY_D)) { lightPos.x += moveSpeed * dt; }
        if (input.KeyDown(GLFW_KEY_Q)) { lightPos.y -= moveSpeed * dt; }
        if (input.KeyDown(GLFW_KEY_E)) { lightPos.y += moveSpeed * dt; }

        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        renderer.Resize(fbw, fbh);
        camera.SetAspect(static_cast<float>(fbw) / static_cast<float>(fbh));

        renderer.BeginFrame(glm::vec4(0.06f, 0.07f, 0.09f, 1.0f));

        const glm::mat4 view = camera.View();
        const glm::mat4 proj = camera.Proj();

        // Draw axes
        axis.Use();
        axis.SetMat4("uView", view);
        axis.SetMat4("uProj", proj);
        axisModel.Draw();

        glDisable(GL_CULL_FACE);
        DrawAxisTickLabels(textRenderer, font, textShader, camera, view, proj, fbw, fbh);
		glEnable(GL_CULL_FACE);

        // Draw cube (textured + point light)
        lit.Use();
        lit.SetMat4("uModel", glm::mat4(1.0f));
        lit.SetMat4("uView", view);
        lit.SetMat4("uProj", proj);
        lit.SetVec3("uCameraPos", camera.Position());
        lit.SetVec3("uLightPos", lightPos);
        lit.SetVec3("uLightColor", lightColor);
        lit.SetFloat("uAmbientStrength", 0.12f);
        lit.SetFloat("uSpecStrength", 0.65f);
        lit.SetFloat("uShininess", 64.0f);
        lit.SetInt("uAlbedo", 0);

        if (hasTex)
        {
            checker.Bind(0);
        }
        else
        {
            // If texture missing, bind 0; shader will sample black.
            glBindTexture(GL_TEXTURE_2D, 0);
        }


        if (models.size() == 0) {
            cubeModel.Draw();
        }

        for (auto& [model, node] : models) {
			const glm::mat4 modelMat = ComputeWorldMatrix(*node);

			lit.SetMat4("uModel", modelMat);

            model.Draw(); 
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
