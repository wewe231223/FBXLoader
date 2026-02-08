#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>

#ifdef USE_GLEW
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Common.h"
#include "FontAtlas.h"
#include "Input.h"
#include "MeshHierarchyBuilder.h"
#include "Model.h"
#include "Renderer.h"
#include "Shader.h"
#include "TextRenderer.h"
#include "Texture.h"
#include "Timer.h"
#include "UfbxAssetLoader.h"

namespace Fs = std::filesystem;

namespace {
    constexpr int WindowWidth{ 1280 };
    constexpr int WindowHeight{ 720 };

    void AppendVertex(asset::VertexAttributes& Vertices, const glm::vec3& Position, const glm::vec3& Normal, const std::array<glm::vec2, 4>& TexCoords, const glm::vec4& Color, const glm::vec3& Tangent, const glm::vec3& Bitangent, const glm::uvec4& BoneIndices, const glm::vec4& BoneWeights) {
        Vertices.Positions.push_back(Position);
        Vertices.Normals.push_back(Normal);
        for (std::size_t Index{ 0 }; Index < Vertices.TexCoords.size(); ++Index) {
            Vertices.TexCoords[Index].push_back(TexCoords[Index]);
        }
        Vertices.Colors.push_back(Color);
        Vertices.Tangents.push_back(Tangent);
        Vertices.Bitangents.push_back(Bitangent);
        Vertices.BoneIndices.push_back(BoneIndices);
        Vertices.BoneWeights.push_back(BoneWeights);
    }

    asset::Model CreateAxisModel() {
        constexpr float AxisLength{ 2000.0f };
        constexpr float TickInterval{ 1.0f };
        constexpr float TickSize{ 0.05f };

        asset::VertexAttributes Vertices{};
        std::vector<std::uint32_t> Indices{};

        const std::size_t EstimatedTicks{ static_cast<std::size_t>((AxisLength * 2.0f / TickInterval) * 3.0f * 2.0f) };
        Vertices.Reserve(6 + EstimatedTicks);
        Indices.reserve(6 + EstimatedTicks);

        const glm::vec3 Normal{ 0.0f, 1.0f, 0.0f };
        const std::array<glm::vec2, 4> TexCoords{ glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } };
        const glm::vec3 Tangent{ 0.0f, 0.0f, 0.0f };
        const glm::vec3 Bitangent{ 0.0f, 0.0f, 0.0f };
        const glm::uvec4 BoneIndices{ 0, 0, 0, 0 };
        const glm::vec4 BoneWeights{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto AddLine = [&](const glm::vec3& Start, const glm::vec3& End, const glm::vec4& StartColor, const glm::vec4& EndColor) {
            const std::uint32_t Base{ static_cast<std::uint32_t>(Vertices.VertexCount()) };
            AppendVertex(Vertices, Start, Normal, TexCoords, StartColor, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, End, Normal, TexCoords, EndColor, Tangent, Bitangent, BoneIndices, BoneWeights);
            Indices.push_back(Base + 0);
            Indices.push_back(Base + 1);
        };

        auto AddTick = [&](const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color) {
            const std::uint32_t Base{ static_cast<std::uint32_t>(Vertices.VertexCount()) };
            AppendVertex(Vertices, Start, Normal, TexCoords, Color, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, End, Normal, TexCoords, Color, Tangent, Bitangent, BoneIndices, BoneWeights);
            Indices.push_back(Base + 0);
            Indices.push_back(Base + 1);
        };

        AddLine(glm::vec3{ -AxisLength, 0.0f, 0.0f }, glm::vec3{ AxisLength, 0.0f, 0.0f }, glm::vec4{ 0.5f, 0.0f, 0.0f, 1.0f }, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });
        AddLine(glm::vec3{ 0.0f, -AxisLength, 0.0f }, glm::vec3{ 0.0f, AxisLength, 0.0f }, glm::vec4{ 0.0f, 0.5f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
        AddLine(glm::vec3{ 0.0f, 0.0f, -AxisLength }, glm::vec3{ 0.0f, 0.0f, AxisLength }, glm::vec4{ 0.0f, 0.0f, 0.5f, 1.0f }, glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f });

        for (float X{ -AxisLength }; X <= AxisLength + 0.0001f; X += TickInterval) {
            if (std::abs(X) < 0.0001f) {
                continue;
            }

            AddTick(glm::vec3{ X, -TickSize, 0.0f }, glm::vec3{ X, TickSize, 0.0f }, glm::vec4{ 0.7f, 0.0f, 0.0f, 1.0f });
        }

        for (float Y{ -AxisLength }; Y <= AxisLength + 0.0001f; Y += TickInterval) {
            if (std::abs(Y) < 0.0001f) {
                continue;
            }

            AddTick(glm::vec3{ -TickSize, Y, 0.0f }, glm::vec3{ TickSize, Y, 0.0f }, glm::vec4{ 0.0f, 0.7f, 0.0f, 1.0f });
        }

        for (float Z{ -AxisLength }; Z <= AxisLength + 0.0001f; Z += TickInterval) {
            if (std::abs(Z) < 0.0001f) {
                continue;
            }

            AddTick(glm::vec3{ 0.0f, -TickSize, Z }, glm::vec3{ 0.0f, TickSize, Z }, glm::vec4{ 0.0f, 0.0f, 0.7f, 1.0f });
        }

        asset::Model ModelInstance{};
        ModelInstance.Create(Vertices, Indices, GL_LINES);
        return ModelInstance;
    }

    asset::Model CreateTexturedCube() {
        asset::VertexAttributes Vertices{};
        Vertices.Reserve(24);

        const glm::vec4 White{ 1.0f };
        const glm::vec3 Tangent{ 0.0f, 0.0f, 0.0f };
        const glm::vec3 Bitangent{ 0.0f, 0.0f, 0.0f };
        const glm::uvec4 BoneIndices{ 0, 0, 0, 0 };
        const glm::vec4 BoneWeights{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto AddFace = [&](const glm::vec3& Normal, const glm::vec3& Position0, const glm::vec3& Position1, const glm::vec3& Position2, const glm::vec3& Position3) {
            std::array<glm::vec2, 4> TexCoords0{ glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } };
            std::array<glm::vec2, 4> TexCoords1{ glm::vec2{ 1.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } };
            std::array<glm::vec2, 4> TexCoords2{ glm::vec2{ 1.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } };
            std::array<glm::vec2, 4> TexCoords3{ glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f } };

            AppendVertex(Vertices, Position0, Normal, TexCoords0, White, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, Position1, Normal, TexCoords1, White, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, Position2, Normal, TexCoords2, White, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, Position3, Normal, TexCoords3, White, Tangent, Bitangent, BoneIndices, BoneWeights);
        };

        const float Size{ 0.5f };

        AddFace(glm::vec3{ 0.0f, 0.0f, 1.0f }, glm::vec3{ -Size, -Size, Size }, glm::vec3{ Size, -Size, Size }, glm::vec3{ Size, Size, Size }, glm::vec3{ -Size, Size, Size });
        AddFace(glm::vec3{ 0.0f, 0.0f, -1.0f }, glm::vec3{ Size, -Size, -Size }, glm::vec3{ -Size, -Size, -Size }, glm::vec3{ -Size, Size, -Size }, glm::vec3{ Size, Size, -Size });
        AddFace(glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec3{ Size, -Size, Size }, glm::vec3{ Size, -Size, -Size }, glm::vec3{ Size, Size, -Size }, glm::vec3{ Size, Size, Size });
        AddFace(glm::vec3{ -1.0f, 0.0f, 0.0f }, glm::vec3{ -Size, -Size, -Size }, glm::vec3{ -Size, -Size, Size }, glm::vec3{ -Size, Size, Size }, glm::vec3{ -Size, Size, -Size });
        AddFace(glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ -Size, Size, Size }, glm::vec3{ Size, Size, Size }, glm::vec3{ Size, Size, -Size }, glm::vec3{ -Size, Size, -Size });
        AddFace(glm::vec3{ 0.0f, -1.0f, 0.0f }, glm::vec3{ -Size, -Size, -Size }, glm::vec3{ Size, -Size, -Size }, glm::vec3{ Size, -Size, Size }, glm::vec3{ -Size, -Size, Size });

        std::vector<std::uint32_t> Indices{};
        Indices.reserve(36);
        for (std::uint32_t FaceIndex{ 0 }; FaceIndex < 6; ++FaceIndex) {
            const std::uint32_t Base{ FaceIndex * 4 };    
            Indices.push_back(Base + 0);
            Indices.push_back(Base + 1);
            Indices.push_back(Base + 2);
            Indices.push_back(Base + 0);
            Indices.push_back(Base + 2);
            Indices.push_back(Base + 3);
        }

        asset::Model ModelInstance{};
        ModelInstance.Create(Vertices, Indices, GL_TRIANGLES);
        return ModelInstance;
    }

    void FramebufferSizeCallback(GLFWwindow* WindowHandle, int Width, int Height) {
        (void)WindowHandle;
        glViewport(0, 0, Width, Height);
    }

    asset::ModelResult OnFileDropped(const std::string& Path, std::vector<std::pair<asset::Model, const asset::ModelNode*>>& Models) {
        Fs::path FilePath{ Path };

        Models.clear();

        asset::UfbxAssetLoader Loader{ asset::GraphicsAPI::OpenGL };
        asset::ModelResult Result{};
        asset::MeshHierarchyBuilder Builder{ Result };

        asset::ISceneNodeVisitor* Visitors[]{ &Builder };

        if (FilePath.extension() != ".fbx" && FilePath.extension() != ".FBX") {
            std::cout << "Wrong file type - " << Path << "\nOnly .fbx files are supported.\n";
            return Result;
        }

        Loader.LoadAndTraverse(Path, { Visitors });

        Result.ForEachDfs([&Models](asset::ModelNode& Node) {
            if (Node.Vertices().Empty()) {
                return;
            }

            asset::Model ModelInstance{};
            ModelInstance.Create(Node.Vertices(), Node.Indices(), GL_TRIANGLES);
            Models.emplace_back(std::move(ModelInstance), &Node);
        });

        std::cout << "[Drop] " << Path << "\n";
        return Result;
    }

    glm::mat4 ComputeWorldMatrix(const asset::ModelNode& Node) {
        glm::mat4 World{ 1.0f };

        std::vector<const asset::ModelNode*> Chain{ Node.GetChildChain() };

        for (std::size_t Index{ 0 }; Index < Chain.size(); ++Index) {
            const asset::ModelNode* Current{ Chain[Index] };
            World = World * Current->GetNodeToParent();
            World = World * Current->GetGeometryToNode();
        }

        return World;
    }

    std::string FindSystemFontTtf() {
        const std::vector<std::string> Candidates{
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/System/Library/Fonts/Supplemental/Helvetica.ttf",
            "/System/Library/Fonts/SFNS.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
        };

        for (const std::string& Path : Candidates) {
            if (Fs::exists(Path)) {
                return Path;
            }
        }
        return {};
    }

    void DrawAxisTickLabels(asset::TextRenderer& TextRendererInstance, const asset::FontAtlas& Font, asset::Shader& TextShader, const asset::Camera& CameraInstance, const glm::mat4& View, const glm::mat4& Projection, int FramebufferWidth, int FramebufferHeight) {
        if (Font.TextureId() == 0) {
            return;
        }

        constexpr float AxisLength{ 15.0f };
        constexpr float TickInterval{ 1.0f };
        constexpr float LabelPx{ 15.0f };
        constexpr float Offset{ 0.10f };

        auto DrawOne = [&](const std::string& Label, const glm::vec3& Position, const glm::vec4& Color) {
            TextRendererInstance.DrawTextBillboard(Font, TextShader, Label, Position, CameraInstance, View, Projection, FramebufferWidth, FramebufferHeight, LabelPx, Color, true);
        };

        for (float X{ -AxisLength }; X <= AxisLength + 0.0001f; X += TickInterval) {
            if (std::abs(X) < 0.0001f) {
                continue;
            }

            const int Value{ static_cast<int>(std::round(X)) };
            DrawOne(std::to_string(Value) + "m", glm::vec3{ X, Offset, 0.0f }, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
        }

        for (float Y{ -AxisLength }; Y <= AxisLength + 0.0001f; Y += TickInterval) {
            if (std::abs(Y) < 0.0001f) {
                continue;
            }

            const int Value{ static_cast<int>(std::round(Y)) };
            DrawOne(std::to_string(Value) + "m", glm::vec3{ Offset, Y, 0.0f }, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
        }

        for (float Z{ -AxisLength }; Z <= AxisLength + 0.0001f; Z += TickInterval) {
            if (std::abs(Z) < 0.0001f) {
                continue;
            }

            const int Value{ static_cast<int>(std::round(Z)) };
            DrawOne(std::to_string(Value) + "m", glm::vec3{ 0.0f, Offset, Z }, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* Window{ glfwCreateWindow(WindowWidth, WindowHeight, "OpenGL Model Viewer", nullptr, nullptr) };
    if (!Window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(Window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return 1;
    }

#ifdef USE_GLEW
    glewExperimental = GL_TRUE;
    const GLenum GlewErr{ glewInit() };
    if (GlewErr != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(GlewErr) << "\n";
        glfwTerminate();
        return 1;
    }
#endif

    asset::Renderer RendererInstance{};
    RendererInstance.Initialize();
    RendererInstance.Resize(WindowWidth, WindowHeight);

    asset::Timer TimerInstance{};

    asset::Input InputHandler{ Window };
    asset::Input::InstallCallbacks(Window, &InputHandler);

    asset::Camera CameraInstance{};
    CameraInstance.SetAspect(static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight));
    CameraInstance.SetTarget(glm::vec3{ 0.0f, 0.0f, 0.0f });
    CameraInstance.SetDistance(5.0f);
    CameraInstance.SetYawPitch(glm::radians(45.0f), glm::radians(-20.0f));

    asset::Shader LitShader{};
    asset::Shader AxisShader{};

    const Fs::path ShaderDir{ Fs::current_path() / "shaders" };
    if (!LitShader.LoadFromFiles((ShaderDir / "lit.vert").string(), (ShaderDir / "lit.frag").string())) {
        std::cerr << "Failed to load lit shader\n";
        return 1;
    }
    if (!AxisShader.LoadFromFiles((ShaderDir / "axis.vert").string(), (ShaderDir / "axis.frag").string())) {
        std::cerr << "Failed to load axis shader\n";
        return 1;
    }

    asset::FontAtlas Font{};
    asset::TextRenderer TextRendererInstance{};
    asset::Shader TextShader{};

    const std::string FontPath{ FindSystemFontTtf() };
    if (!FontPath.empty()) {
        if (!Font.LoadFromTtfFile(FontPath, 32.0f, 512, 512, 32, 96)) {
            std::cerr << "Failed to load font atlas: " << FontPath << "\n";
        }
    }
    else {
        std::cerr << "No system font found. Provide a .ttf.\n";
    }

    std::cout << "Font path: " << FontPath << "\n";

    const bool FontOk{ (!FontPath.empty()) && Font.LoadFromTtfFile(FontPath, 32.0f, 512, 512, 32, 96) };

    std::cout << "Font loaded: " << (FontOk ? "OK" : "FAILED") << " tex=" << Font.TextureId() << " atlas=" << Font.AtlasWidth() << "x" << Font.AtlasHeight() << " bake=" << Font.BakePixelHeight() << "\n";

    if (!TextShader.LoadFromFiles((ShaderDir / "text.vert").string(), (ShaderDir / "text.frag").string())) {
        std::cerr << "Failed to load text shader\n";
    }

    TextRendererInstance.Initialize();

    asset::Texture2D Checker{};
    const Fs::path AssetPath{ Fs::current_path() / "assets" / "checker.png" };
    const bool HasTexture{ Checker.LoadFromFile(AssetPath.string(), true) };

    asset::Model AxisModel{ CreateAxisModel() };
    asset::Model CubeModel{ CreateTexturedCube() };

    std::vector<std::pair<asset::Model, const asset::ModelNode*>> Models{};
    asset::ModelResult Result{};

    glm::vec3 LightPosition{ 2.0f, 1.5f, 2.0f };
    glm::vec3 LightColor{ 1.0f, 1.0f, 1.0f };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(Window)) {
        TimerInstance.Tick();
        InputHandler.NewFrame();
        InputHandler.Poll();

        {
            const auto Dropped{ InputHandler.ConsumeDroppedFiles() };
            for (const auto& Path : Dropped) {
                Result = OnFileDropped(Path, Models);
            }

            if (!Dropped.empty() && !Models.empty()) {
                const asset::Model::Bounds& Bounds{ Models[0].first.GetBounds() };
                const float Radius{ Models[0].first.GetBoundingSphereRadius() };
                //CameraInstance.FrameBounds(Bounds.Center(), Radius, 1.25f);
            }
        }

        if (InputHandler.KeyPressed(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(Window, GLFW_TRUE);
        }

        if (InputHandler.MouseDown(GLFW_MOUSE_BUTTON_LEFT)) {
            const glm::vec2 Delta{ InputHandler.MouseDelta() };
            const float RotationSpeed{ 0.006f };
            CameraInstance.AddYawPitch(-Delta.x * RotationSpeed, -Delta.y * RotationSpeed);
        }
        {
            const float Scroll{ InputHandler.ScrollDelta() };
            if (Scroll != 0.0f) {
                CameraInstance.AddZoom(Scroll * 0.5f);
            }
        }

        const float MoveSpeed{ 2.5f };
        const float DeltaTime{ TimerInstance.DeltaSeconds() };
        if (InputHandler.KeyDown(GLFW_KEY_W)) {
            LightPosition.z += MoveSpeed * DeltaTime;
        }
        if (InputHandler.KeyDown(GLFW_KEY_S)) {
            LightPosition.z -= MoveSpeed * DeltaTime;
        }
        if (InputHandler.KeyDown(GLFW_KEY_A)) {
            LightPosition.x -= MoveSpeed * DeltaTime;
        }
        if (InputHandler.KeyDown(GLFW_KEY_D)) {
            LightPosition.x += MoveSpeed * DeltaTime;
        }
        if (InputHandler.KeyDown(GLFW_KEY_Q)) {
            LightPosition.y -= MoveSpeed * DeltaTime;
        }
        if (InputHandler.KeyDown(GLFW_KEY_E)) {
            LightPosition.y += MoveSpeed * DeltaTime;
        }

        int FramebufferWidth{ 0 };
        int FramebufferHeight{ 0 };
        glfwGetFramebufferSize(Window, &FramebufferWidth, &FramebufferHeight);
        RendererInstance.Resize(FramebufferWidth, FramebufferHeight);
        CameraInstance.SetAspect(static_cast<float>(FramebufferWidth) / static_cast<float>(FramebufferHeight));

        RendererInstance.BeginFrame(glm::vec4{ 0.06f, 0.07f, 0.09f, 1.0f });

        const glm::mat4 View{ CameraInstance.View() };
        const glm::mat4 Projection{ CameraInstance.Proj() };

        AxisShader.Use();
        AxisShader.SetMat4("uView", View);
        AxisShader.SetMat4("uProj", Projection);
        AxisModel.Draw();

        glDisable(GL_CULL_FACE);
        DrawAxisTickLabels(TextRendererInstance, Font, TextShader, CameraInstance, View, Projection, FramebufferWidth, FramebufferHeight);
        glEnable(GL_CULL_FACE);

        LitShader.Use();
        LitShader.SetMat4("uModel", glm::mat4{ 1.0f });
        LitShader.SetMat4("uView", View);
        LitShader.SetMat4("uProj", Projection);
        LitShader.SetVec3("uCameraPos", CameraInstance.Position());
        LitShader.SetVec3("uLightPos", LightPosition);
        LitShader.SetVec3("uLightColor", LightColor);
        LitShader.SetFloat("uAmbientStrength", 0.12f);
        LitShader.SetFloat("uSpecStrength", 0.65f);
        LitShader.SetFloat("uShininess", 64.0f);
        LitShader.SetInt("uAlbedo", 0);

        if (HasTexture) {
            Checker.Bind(0);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        if (Models.empty()) {
            CubeModel.Draw();
        }

        for (auto& Entry : Models) {
            asset::Model& ModelInstance{ Entry.first };
            const asset::ModelNode* Node{ Entry.second };
            const glm::mat4 ModelMatrix{ ComputeWorldMatrix(*Node) };
            LitShader.SetMat4("uModel", ModelMatrix);
            ModelInstance.Draw();
        }

        glfwSwapBuffers(Window);
    }

    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
