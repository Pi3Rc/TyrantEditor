    /*
    hello, this script is full of bullshit and many mistakes,
    i stole a lot of code that i kinda understand from the raylib example projects.
    i mostly used the raylib examples and the raylib cheatsheet

    enjoy reading my (not rlly mine but yeah) script :thumbs_up:
    */

    #include "raylib.h"
    #include "rlgl.h"
    #include "raymath.h"

    #include <iostream>
    #include <vector>

    #pragma region imgui
    #include "imgui.h"
    #include "rlImGui.h"
    #include "imguiThemes.h"
    #pragma endregion

    static TextureCubemap GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format);

    class object
    {
        public:
            std::string name = "New object";
            Model model;
            Vector3 position = {0.0f, 0.0f, 0.0f};  
            Color colorTint = WHITE;
            float scale = 1.0f;
            float transp = 255.0f;
    };

    int main(void)
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        const int screenWidth = 800;
        const int screenHeight = 450;
        InitWindow(screenWidth, screenHeight, "Tyrant Scene Editor");

        Camera3D camera = { 0 };
        camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
        camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
        camera.fovy = 45.0f;                                // Camera field-of-view Y
        camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
        bool cursorLock = false;

        std::vector<object> sceneObjects;
        Ray ray = { 0 };
        int selectedObject = -1;

        SetTargetFPS(144);

        //ui
        bool UIActive = true;

        //skybox
        Mesh skyboxCube = GenMeshCube(100.0f, 100.0f, 100.0f); //skybox size basically
        Model skybox = LoadModelFromMesh(skyboxCube); //load skybox on the cube

        skybox.materials[0].shader = LoadShader("resources/Shaders/skybox.vs","resources/Shaders/skybox.fs");
        bool useHDR = false;
        SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "environmentMap"), (int[1]){ MATERIAL_MAP_CUBEMAP }, SHADER_UNIFORM_INT);
        SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "doGamma"), (int[1]){ useHDR? 1 : 0 }, SHADER_UNIFORM_INT);
        SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "vflipped"), (int[1]){ useHDR? 1 : 0 }, SHADER_UNIFORM_INT);

        Shader shdrCubemap = LoadShader("resources/Shaders/cubemap.vs","resources/Shaders/cubemap.fs");

        SetShaderValue(shdrCubemap, GetShaderLocation(shdrCubemap, "equirectangularMap"), (int[1]){ 0 }, SHADER_UNIFORM_INT);

        char skyboxFileName[256] = "resources/cubemap/this.png"; // get default skybox name
        if (useHDR)
        {
            Texture2D panorama = LoadTexture(skyboxFileName);

            skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = GenTextureCubemap(shdrCubemap, panorama, 1024, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

            UnloadTexture(panorama);
        }
        else
        {
            Image image = LoadImage("resources/cubemap/this.png");
            
            skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(image, CUBEMAP_LAYOUT_AUTO_DETECT);
            UnloadImage(image);
        }

        DisableCursor();    // Limit cursor to relative movement inside the window

    #pragma region imgui
        rlImGuiSetup(true);

        imguiThemes::gray();

        ImGuiIO &io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.FontGlobalScale = 2;

        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            //style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 0.5f;
            style.Colors[ImGuiCol_DockingEmptyBg].w = 0.f;
        }

    #pragma endregion


        //game loop
        while (!WindowShouldClose())
        {   
                // quality of life section starts here
            //mouse locking
            if (IsKeyPressed(KEY_GRAVE)) 
            {
                cursorLock = true;
                DisableCursor();
            }
            if (IsKeyPressed(KEY_TAB)) 
            {
                cursorLock = false;
                EnableCursor();
            }

            //teleport to object
            if(IsKeyPressed(KEY_F)) 
            {
                object& obj = sceneObjects[selectedObject];
                camera.position = obj.position;
            }
            //hide all ui
            if(IsKeyPressed(KEY_L) && UIActive) 
            {
                UIActive = false;
            }
            else if(IsKeyPressed(KEY_L) && !UIActive) 
            {
                UIActive = true;
            }
            //reset camera rotation
            if(IsKeyPressed(KEY_R)) 
            {
                camera.up = {0.0f, 1.0f, 0.0f};
            }
                // quality of life section ends here

            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !cursorLock && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
            {
                selectedObject = -1;
                ray = GetMouseRay(GetMousePosition(), camera);
                

                for (int i = 0; i < sceneObjects.size(); i++)
                {
                    BoundingBox box = GetModelBoundingBox(sceneObjects[i].model);
                    box.min = Vector3Add(box.min, sceneObjects[i].position);
                    box.max = Vector3Add(box.max, sceneObjects[i].position);
                    RayCollision hit = GetRayCollisionBox(ray, box);
                    if (hit.hit)
                    {
                        selectedObject = i;
                        break;
                    }
                }
            }

            if (IsFileDropped())
            {
                FilePathList droppedFiles = LoadDroppedFiles();

                if (droppedFiles.count == 1)
                {
                    if (IsFileExtension(droppedFiles.paths[0], ".png;.jpg;.hdr;.bmp;.tga"))
                    {
                        // Unload current cubemap texture to load new one
                        UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
                        TextCopy(skyboxFileName, droppedFiles.paths[0]);

                        if (useHDR)
                        {
                            // Load HDR panorama (sphere) texture
                            Texture2D panorama = LoadTexture(droppedFiles.paths[0]);

                            // Generate cubemap from panorama texture
                            skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = GenTextureCubemap(shdrCubemap, panorama, 1024, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

                            UnloadTexture(panorama);    // Texture not required anymore, cubemap already generated
                        }
                        else
                        {
                            Image image = LoadImage(droppedFiles.paths[0]);
                            skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(image, CUBEMAP_LAYOUT_AUTO_DETECT);
                            UnloadImage(image);
                        }

                    }else if (IsFileExtension(droppedFiles.paths[0], ".glb")) 
                    { //loading models when u drop them
                        object obj;
                        obj.name = droppedFiles.paths[0];
                        obj.model = LoadModel(droppedFiles.paths[0]);
                        obj.position = {0.0f, 0.0f, 0.0f};
                        sceneObjects.push_back(obj);
                    }
                }
                UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
            }

            BeginDrawing();
            
            ClearBackground(BLACK);

            if (cursorLock) 
            {
                UpdateCamera(&camera, CAMERA_FREE);
            }

            BeginMode3D(camera);
                
                rlDisableBackfaceCulling();
                rlDisableDepthMask();
                DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, WHITE);
                rlEnableBackfaceCulling();
                rlEnableDepthMask();

                //model loading starts here
                for (int i = 0; i < sceneObjects.size(); i++)
                {
                    Color tint = (i == selectedObject) ? GRAY : sceneObjects[i].colorTint;
                    tint.a = (unsigned char)(sceneObjects[i].transp);
                    DrawModel(sceneObjects[i].model, sceneObjects[i].position, sceneObjects[i].scale, tint);
                }
                //model loading ends here

                DrawGrid(10, 1.0f);

                EndMode3D();

        #pragma region imgui
        if (UIActive) 
        {
            //all UI should be written here
            DrawText(TextFormat("Number of objects: %d", sceneObjects.size()), 10, 10, 20, WHITE);
            DrawText(GetFileName(skyboxFileName), 10, 30, 20, WHITE);

            rlImGuiBegin();

            ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
            ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
            ImGui::PopStyleColor(2);
        #pragma endregion


            ImGui::Begin("Edit");

            ImGui::Text("Models");

            if (ImGui::Button("Delete") && selectedObject != -1)
            {
                std::cout << "Delete works, hiiiii do u heaarrrr meeee... wait.. it should be do u read me??";
                UnloadModel(sceneObjects[selectedObject].model);
                sceneObjects.erase(sceneObjects.begin() + selectedObject);
                selectedObject = -1;
            }

            if (selectedObject != -1)
            {
                object& obj = sceneObjects[selectedObject];
                
                ImGui::Text("Selected: %s", GetFileName(obj.name.c_str()));
                ImGui::SliderFloat3("Position", &obj.position.x, -50.0f, 50.0f);
                ImGui::SliderFloat("Scale", &obj.scale, 0.1f, 10.0f);
                ImGui::SliderFloat("Transparency", &obj.transp, 0.0f, 255.0f);
            }
            
            ImGui::End();

        #pragma region imgui
            rlImGuiEnd();

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
        }

        #pragma endregion

            EndDrawing();
        }


    #pragma region imgui
        rlImGuiShutdown();
    #pragma endregion

        UnloadShader(skybox.materials[0].shader);
        UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);

        UnloadModel(skybox);        // Unload skybox model

        for (auto& obj : sceneObjects)
        {
            UnloadModel(obj.model);
        }

        CloseWindow();

        return 0;
    }

    static TextureCubemap GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format)
    {
        TextureCubemap cubemap = { 0 };

        rlDisableBackfaceCulling();     // Disable backface culling to render inside the cube

        // STEP 1: Setup framebuffer
        //------------------------------------------------------------------------------------------
        unsigned int rbo = rlLoadTextureDepth(size, size, true);
        cubemap.id = rlLoadTextureCubemap(0, size, format);

        unsigned int fbo = rlLoadFramebuffer(800, 450);
        rlFramebufferAttach(fbo, rbo, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
        rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X, 0);

        // Check if framebuffer is complete with attachments (valid)
        if (rlFramebufferComplete(fbo)) TraceLog(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", fbo);
        //------------------------------------------------------------------------------------------

        // STEP 2: Draw to framebuffer
        //------------------------------------------------------------------------------------------
        // NOTE: Shader is used to convert HDR equirectangular environment map to cubemap equivalent (6 faces)
        rlEnableShader(shader.id);

        // Define projection matrix and send it to shader
        Matrix matFboProjection = MatrixPerspective(90.0*DEG2RAD, 1.0, 0.01, 1000);
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matFboProjection);

        // Define view matrix for every side of the cubemap
        Matrix fboViews[6] = {
            MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  1.0f,  0.0f,  0.0f }, (Vector3){ 0.0f, -1.0f,  0.0f }),
            MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ -1.0f,  0.0f,  0.0f }, (Vector3){ 0.0f, -1.0f,  0.0f }),
            MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f,  1.0f,  0.0f }, (Vector3){ 0.0f,  0.0f,  1.0f }),
            MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f, -1.0f,  0.0f }, (Vector3){ 0.0f,  0.0f, -1.0f }),
            MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f,  0.0f,  1.0f }, (Vector3){ 0.0f, -1.0f,  0.0f }),
            MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f,  0.0f, -1.0f }, (Vector3){ 0.0f, -1.0f,  0.0f })
        };

        rlViewport(0, 0, size, size);   // Set viewport to current fbo dimensions

        // Activate and enable texture for drawing to cubemap faces
        rlActiveTextureSlot(0);
        rlEnableTexture(panorama.id);

        for (int i = 0; i < 6; i++)
        {
            // Set the view matrix for the current cube face
            rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], fboViews[i]);

            // Select the current cubemap face attachment for the fbo
            // WARNING: This function by default enables->attach->disables fbo!!!
            rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i, 0);
            rlEnableFramebuffer(fbo);

            // Load and draw a cube, it uses the current enabled texture
            rlClearScreenBuffers();
            rlLoadDrawCube();
        }
        //------------------------------------------------------------------------------------------

        // STEP 3: Unload framebuffer and reset state
        //------------------------------------------------------------------------------------------
        rlDisableShader();          // Unbind shader
        rlDisableTexture();         // Unbind texture
        rlDisableFramebuffer();     // Unbind framebuffer
        rlUnloadFramebuffer(fbo);   // Unload framebuffer (and automatically attached depth texture/renderbuffer)

        // Reset viewport dimensions to default
        rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());
        rlEnableBackfaceCulling();
        //------------------------------------------------------------------------------------------

        cubemap.width = size;
        cubemap.height = size;
        cubemap.mipmaps = 1;
        cubemap.format = format;

        return cubemap;
    }