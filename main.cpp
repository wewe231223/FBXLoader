#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
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
#include "AssetBinaryReader.h"
#include "AssetBinaryWriter.h"
#include "AssetBundle.h"
#include "Common.h"
#include "FbxAssetImporter.h"
#include "FontAtlas.h"
#include "Input.h"
#include "Model.h"
#include "Renderer.h"
#include "Shader.h"
#include "TextRenderer.h"
#include "Texture.h"
#include "Timer.h"
#include "ViewerMath.h"

namespace Fs = std::filesystem;

namespace {
    constexpr int WindowWidth{ 1280 };
    constexpr int WindowHeight{ 720 };

    void AppendVertex(asset::VertexAttributes& Vertices, const asset::Vec3& Position, const asset::Vec3& Normal, const std::array<asset::Vec2, 4>& TexCoords, const asset::Vec4& Color, const asset::Vec3& Tangent, const asset::Vec3& Bitangent, const asset::UVec4& BoneIndices, const asset::Vec4& BoneWeights) {
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

    struct ModelEntry final {
    public:
        asset::Model Model{};
        const asset::ModelNode* Node{ nullptr };
        std::optional<std::size_t> MaterialIndex{};
    };

    std::optional<std::string> FindMaterialTextureName(const asset::Material& MaterialData) {
        const std::array<asset::MaterialType, 6> CandidateTypes{
            asset::MaterialType::BaseColorMap,
            asset::MaterialType::DiffuseColorMap,
            asset::MaterialType::DiffuseFactorMap,
            asset::MaterialType::EmissionColorMap,
            asset::MaterialType::EmissionColorPbrMap,
            asset::MaterialType::OpacityMap
        };
        for (const asset::MaterialType Type : CandidateTypes) {
            for (const asset::MaterialProperty& Property : MaterialData.Properties) {
                if (Property.Type != Type) {
                    continue;
                }
                if (Property.Data.GetKind() != asset::MaterialMapKind::String) {
                    continue;
                }
                const std::string& Name{ Property.Data.GetString() };
                if (!Name.empty()) {
                    return Name;
                }
            }
        }
        return std::nullopt;
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

        const asset::Vec3 Normal{ 0.0f, 1.0f, 0.0f };
        const std::array<asset::Vec2, 4> TexCoords{ asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f } };
        const asset::Vec3 Tangent{ 0.0f, 0.0f, 0.0f };
        const asset::Vec3 Bitangent{ 0.0f, 0.0f, 0.0f };
        const asset::UVec4 BoneIndices{ 0, 0, 0, 0 };
        const asset::Vec4 BoneWeights{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto AddLine = [&](const asset::Vec3& Start, const asset::Vec3& End, const asset::Vec4& StartColor, const asset::Vec4& EndColor) {
            const std::uint32_t Base{ static_cast<std::uint32_t>(Vertices.VertexCount()) };
            AppendVertex(Vertices, Start, Normal, TexCoords, StartColor, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, End, Normal, TexCoords, EndColor, Tangent, Bitangent, BoneIndices, BoneWeights);
            Indices.push_back(Base + 0);
            Indices.push_back(Base + 1);
        };

        auto AddTick = [&](const asset::Vec3& Start, const asset::Vec3& End, const asset::Vec4& Color) {
            const std::uint32_t Base{ static_cast<std::uint32_t>(Vertices.VertexCount()) };
            AppendVertex(Vertices, Start, Normal, TexCoords, Color, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, End, Normal, TexCoords, Color, Tangent, Bitangent, BoneIndices, BoneWeights);
            Indices.push_back(Base + 0);
            Indices.push_back(Base + 1);
        };

        AddLine(asset::Vec3{ -AxisLength, 0.0f, 0.0f }, asset::Vec3{ AxisLength, 0.0f, 0.0f }, asset::Vec4{ 0.5f, 0.0f, 0.0f, 1.0f }, asset::Vec4{ 1.0f, 0.0f, 0.0f, 1.0f });
        AddLine(asset::Vec3{ 0.0f, -AxisLength, 0.0f }, asset::Vec3{ 0.0f, AxisLength, 0.0f }, asset::Vec4{ 0.0f, 0.5f, 0.0f, 1.0f }, asset::Vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
        AddLine(asset::Vec3{ 0.0f, 0.0f, -AxisLength }, asset::Vec3{ 0.0f, 0.0f, AxisLength }, asset::Vec4{ 0.0f, 0.0f, 0.5f, 1.0f }, asset::Vec4{ 0.0f, 0.0f, 1.0f, 1.0f });

        for (float X{ -AxisLength }; X <= AxisLength + 0.0001f; X += TickInterval) {
            if (std::abs(X) < 0.0001f) {
                continue;
            }

            AddTick(asset::Vec3{ X, -TickSize, 0.0f }, asset::Vec3{ X, TickSize, 0.0f }, asset::Vec4{ 0.7f, 0.0f, 0.0f, 1.0f });
        }

        for (float Y{ -AxisLength }; Y <= AxisLength + 0.0001f; Y += TickInterval) {
            if (std::abs(Y) < 0.0001f) {
                continue;
            }

            AddTick(asset::Vec3{ -TickSize, Y, 0.0f }, asset::Vec3{ TickSize, Y, 0.0f }, asset::Vec4{ 0.0f, 0.7f, 0.0f, 1.0f });
        }

        for (float Z{ -AxisLength }; Z <= AxisLength + 0.0001f; Z += TickInterval) {
            if (std::abs(Z) < 0.0001f) {
                continue;
            }

            AddTick(asset::Vec3{ 0.0f, -TickSize, Z }, asset::Vec3{ 0.0f, TickSize, Z }, asset::Vec4{ 0.0f, 0.0f, 0.7f, 1.0f });
        }

        asset::Model ModelInstance{};
        ModelInstance.Create(Vertices, Indices, GL_LINES);
        return ModelInstance;
    }

    asset::Model CreateTexturedCube() {
        asset::VertexAttributes Vertices{};
        Vertices.Reserve(24);

        const asset::Vec4 White{ 1.0f, 1.0f, 1.0f, 1.0f };
        const asset::Vec3 Tangent{ 0.0f, 0.0f, 0.0f };
        const asset::Vec3 Bitangent{ 0.0f, 0.0f, 0.0f };
        const asset::UVec4 BoneIndices{ 0, 0, 0, 0 };
        const asset::Vec4 BoneWeights{ 0.0f, 0.0f, 0.0f, 0.0f };

        auto AddFace = [&](const asset::Vec3& Normal, const asset::Vec3& Position0, const asset::Vec3& Position1, const asset::Vec3& Position2, const asset::Vec3& Position3) {
            std::array<asset::Vec2, 4> TexCoords0{ asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f } };
            std::array<asset::Vec2, 4> TexCoords1{ asset::Vec2{ 1.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f } };
            std::array<asset::Vec2, 4> TexCoords2{ asset::Vec2{ 1.0f, 1.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f } };
            std::array<asset::Vec2, 4> TexCoords3{ asset::Vec2{ 0.0f, 1.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f }, asset::Vec2{ 0.0f, 0.0f } };

            AppendVertex(Vertices, Position0, Normal, TexCoords0, White, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, Position1, Normal, TexCoords1, White, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, Position2, Normal, TexCoords2, White, Tangent, Bitangent, BoneIndices, BoneWeights);
            AppendVertex(Vertices, Position3, Normal, TexCoords3, White, Tangent, Bitangent, BoneIndices, BoneWeights);
        };

        const float Size{ 0.5f };

        AddFace(asset::Vec3{ 0.0f, 0.0f, 1.0f }, asset::Vec3{ -Size, -Size, Size }, asset::Vec3{ Size, -Size, Size }, asset::Vec3{ Size, Size, Size }, asset::Vec3{ -Size, Size, Size });
        AddFace(asset::Vec3{ 0.0f, 0.0f, -1.0f }, asset::Vec3{ Size, -Size, -Size }, asset::Vec3{ -Size, -Size, -Size }, asset::Vec3{ -Size, Size, -Size }, asset::Vec3{ Size, Size, -Size });
        AddFace(asset::Vec3{ 1.0f, 0.0f, 0.0f }, asset::Vec3{ Size, -Size, Size }, asset::Vec3{ Size, -Size, -Size }, asset::Vec3{ Size, Size, -Size }, asset::Vec3{ Size, Size, Size });
        AddFace(asset::Vec3{ -1.0f, 0.0f, 0.0f }, asset::Vec3{ -Size, -Size, -Size }, asset::Vec3{ -Size, -Size, Size }, asset::Vec3{ -Size, Size, Size }, asset::Vec3{ -Size, Size, -Size });
        AddFace(asset::Vec3{ 0.0f, 1.0f, 0.0f }, asset::Vec3{ -Size, Size, Size }, asset::Vec3{ Size, Size, Size }, asset::Vec3{ Size, Size, -Size }, asset::Vec3{ -Size, Size, -Size });
        AddFace(asset::Vec3{ 0.0f, -1.0f, 0.0f }, asset::Vec3{ -Size, -Size, -Size }, asset::Vec3{ Size, -Size, -Size }, asset::Vec3{ Size, -Size, Size }, asset::Vec3{ -Size, -Size, Size });

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

    std::vector<std::string> LoadListEntries(const std::string& ListPath) {
        std::vector<std::string> Entries{};
        std::ifstream Input{ ListPath };
        if (!Input.is_open()) {
            return Entries;
        }
        std::string Line{};
        while (std::getline(Input, Line)) {
            if (!Line.empty()) {
                Entries.push_back(Line);
            }
        }
        return Entries;
    }

    Fs::path MakeBinaryPath(const Fs::path& FbxPath) {
        Fs::path OutputPath{ FbxPath };
        OutputPath.replace_extension(".fbxbin");
        return OutputPath;
    }

    bool ConvertFbxToBinary(const Fs::path& FbxPath, const Fs::path& BinaryPath) {
        asset::FbxAssetImporter Importer{ asset::GraphicsAPI::OpenGL };
        asset::AssetBundle Bundle{ Importer.LoadFromFile(FbxPath.string()) };
        asset::AssetBinaryWriter Writer{};
        return Writer.WriteToFile(BinaryPath.string(), Bundle);
    }

    void BuildMaterialTextures(const std::vector<asset::Material>& Materials, std::vector<asset::Texture2D>& MaterialTextures) {
        MaterialTextures.clear();
        MaterialTextures.resize(Materials.size());
#ifndef _DEBUG
		const Fs::path TextureRoot{ Fs::current_path() / "images" };
#else 
        const Fs::path TextureRoot{ Fs::current_path() / "Asset" / "images" };
#endif 
        for (std::size_t Index{ 0 }; Index < Materials.size(); ++Index) {
            std::optional<std::string> TextureName{ FindMaterialTextureName(Materials[Index]) };
            if (!TextureName.has_value()) {
                continue;
            }
            const Fs::path TexturePath{ TextureRoot / TextureName.value() };
            MaterialTextures[Index].LoadFromFile(TexturePath.string(), true);
        }
    }

    void BuildModelEntries(const asset::ModelResult& Result, std::vector<ModelEntry>& Models) {
        Models.clear();
        const asset::ModelNode* Root{ Result.GetRoot() };
        if (Root == nullptr) {
            return;
        }
        std::vector<const asset::ModelNode*> Stack{};
        Stack.push_back(Root);
        while (!Stack.empty()) {
            const asset::ModelNode* Node{ Stack.back() };
            Stack.pop_back();
            if (!Node->Vertices().Empty()) {
                asset::Model ModelInstance{};
                ModelInstance.Create(Node->Vertices(), Node->Indices(), GL_TRIANGLES);
                ModelEntry Entry{};
                Entry.Model = std::move(ModelInstance);
                Entry.Node = Node;
                const std::vector<std::size_t>& MaterialIndices{ Node->GetMaterialIndices() };
                if (!MaterialIndices.empty()) {
                    Entry.MaterialIndex = MaterialIndices.front();
                }
                Models.push_back(std::move(Entry));
            }
            const std::vector<asset::ModelNode*>& Children{ Node->GetChildren() };
            for (std::size_t Index{ Children.size() }; Index > 0; --Index) {
                Stack.push_back(Children[Index - 1]);
            }
        }
    }

    bool LoadBinaryAsset(const std::string& Path, asset::AssetBundle& Bundle, std::vector<ModelEntry>& Models, std::vector<asset::Texture2D>& MaterialTextures) {
        Fs::path FilePath{ Path };
        if (FilePath.extension() != ".fbxbin") {
            std::cout << "Wrong file type - " << Path << "\nOnly .fbxbin files are supported.\n";
            return false;
        }
        asset::AssetBinaryReader Reader{};
        if (!Reader.ReadFromFile(Path, Bundle)) {
            std::cout << "Failed to load binary asset: " << Path << "\n";
            return false;
        }
        BuildMaterialTextures(Bundle.GetMaterials(), MaterialTextures);
        BuildModelEntries(Bundle.GetModelResult(), Models);
        std::cout << "[Drop] " << Path << "\n";
        return true;
    }

    glm::mat4 ComputeWorldMatrix(const asset::ModelNode& Node) {
        glm::mat4 World{ 1.0f };

        std::vector<const asset::ModelNode*> Chain{ Node.GetChildChain() };

        for (std::size_t Index{ 0 }; Index < Chain.size(); ++Index) {
            const asset::ModelNode* Current{ Chain[Index] };
            World = World * asset::ToGlmMat4(Current->GetNodeToParent());
            World = World * asset::ToGlmMat4(Current->GetGeometryToNode());
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

int main(int ArgCount, char** ArgValues) {
#ifndef _DEBUG
    if (ArgCount > 1) {
        const std::string ListPath{ ArgValues[1] };
        const std::vector<std::string> Entries{ LoadListEntries(ListPath) };
        if (Entries.empty()) {
            std::cout << "No entries found in list file: " << ListPath << "\n";
        }
        const Fs::path BaseDir{ Fs::path{ ListPath }.parent_path() };
        for (std::size_t Index{ 0 }; Index < Entries.size(); ++Index) {
            Fs::path FbxPath{ Entries[Index] };
            if (FbxPath.is_relative()) {
                FbxPath = BaseDir / FbxPath;
            }
            if (!Fs::exists(FbxPath)) {
                std::cout << "Missing FBX file: " << FbxPath.string() << "\n";
                continue;
            }
            const std::string Extension{ FbxPath.extension().string() };
            if (Extension != ".fbx" && Extension != ".FBX") {
                std::cout << "Skipping non-FBX entry: " << FbxPath.string() << "\n";
                continue;
            }
            const Fs::path BinaryPath{ MakeBinaryPath(FbxPath) };
            if (ConvertFbxToBinary(FbxPath, BinaryPath)) {
                std::cout << "Binary saved: " << BinaryPath.string() << "\n";
            }
            else {
                std::cout << "Binary save failed: " << BinaryPath.string() << "\n";
            }
        }
    }
#else 
    const std::string ListPath{ "Asset/list.txt" };
    const std::vector<std::string> Entries{ LoadListEntries(ListPath) };
    if (Entries.empty()) {
        std::cout << "No entries found in list file: " << ListPath << "\n";
    }
    const Fs::path BaseDir{ Fs::path{ ListPath }.parent_path() };
    for (std::size_t Index{ 0 }; Index < Entries.size(); ++Index) {
        Fs::path FbxPath{ Entries[Index] };
        if (FbxPath.is_relative()) {
            FbxPath = BaseDir / FbxPath;
        }
        if (!Fs::exists(FbxPath)) {
            std::cout << "Missing FBX file: " << FbxPath.string() << "\n";
            continue;
        }
        const std::string Extension{ FbxPath.extension().string() };
        if (Extension != ".fbx" && Extension != ".FBX") {
            std::cout << "Skipping non-FBX entry: " << FbxPath.string() << "\n";
            continue;
        }
        const Fs::path BinaryPath{ MakeBinaryPath(FbxPath) };
        if (ConvertFbxToBinary(FbxPath, BinaryPath)) {
            std::cout << "Binary saved: " << BinaryPath.string() << "\n";
        }
        else {
            std::cout << "Binary save failed: " << BinaryPath.string() << "\n";
        }
    }
#endif 

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

    std::vector<ModelEntry> Models{};
    std::vector<asset::Texture2D> MaterialTextures{};
    asset::AssetBundle Bundle{};

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
                LoadBinaryAsset(Path, Bundle, Models, MaterialTextures);
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
        LitShader.SetFloat("uAmbientStrength", 0.25f);
        LitShader.SetFloat("uSpecStrength", 0.65f);
        LitShader.SetFloat("uShininess", 64.0f);
        LitShader.SetInt("uAlbedo", 0);

        if (Models.empty()) {
            if (HasTexture) {
                Checker.Bind(0);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            CubeModel.Draw();
        }

        for (ModelEntry& Entry : Models) {
            asset::Model& ModelInstance{ Entry.Model };
            const asset::ModelNode* Node{ Entry.Node };
            const glm::mat4 ModelMatrix{ ComputeWorldMatrix(*Node) };
            LitShader.SetMat4("uModel", ModelMatrix);
            bool BoundTexture{ false };
            if (Entry.MaterialIndex.has_value()) {
                const std::size_t MaterialIndex{ Entry.MaterialIndex.value() };
                if (MaterialIndex < MaterialTextures.size() && MaterialTextures[MaterialIndex].Id() != 0) {
                    MaterialTextures[MaterialIndex].Bind(0);
                    BoundTexture = true;
                }
            }
            if (!BoundTexture) {
                if (HasTexture) {
                    Checker.Bind(0);
                }
                else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            ModelInstance.Draw();
        }

        glfwSwapBuffers(Window);
    }

    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
