Rule("Assets/Shaders/(.*)_MTL%.shd", {
    Parse = function(path, name)
        return {string.format("Shaders/%s.metal", name)}, {path}
    end,
    Execute = function(inputPaths, outputPaths)
        print("Compiling " .. inputPaths[1])
        local status, stdout, stderr = RunProcess("Tools/MTLShaderCompiler",
                                                  inputPaths[1], outputPaths[1])
        if status ~= 0 then
            return false, stderr
        end
        return true, nil
    end
})

ContentDir("Shaders")
DataDir("Assets")
Manifest("AssetManifest.txt")
