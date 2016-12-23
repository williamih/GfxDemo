function JoinPath(a, b)
    if a == "" then
        return b
    end
    return a.."/"..b
end

function ExtractDirectory(path)
    local i = #path
    while i > 1 do
        local c = string.sub(path, i, i)
        if c == "/" or c == "\\" then
            return string.sub(path, 1, i - 1)
        end
        i = i - 1
    end
    return ""
end

function ResolveRelativePath(currentDir, path)
    local fullPath = currentDir
    for component in string.gmatch(path, "([^/]+)/?") do
        if component == ".." then
            fullPath = ExtractDirectory(fullPath)
        elseif component ~= "." then
            fullPath = JoinPath(fullPath, component)
        end
    end
    return fullPath
end

function ParseShaderIncludes(currentDir, contents)
    local result = {}
    for line in string.gmatch(contents, "[^\n\r]+") do
        local _, _, relativePath = string.find(line, '#include%s+"(.+)"')
        if relativePath ~= nil then
            result[#result+1] = ResolveRelativePath(currentDir, relativePath)
        end
    end
    return result
end

Rule("Assets/Shaders/(.*)_MTL%.shd", {
    Parse = function(path, name)
        local inputs = {string.format("Shaders/%s.metal", name)}
        local outputs = {path}
        return inputs, outputs, function(inputFilePath, contents)
            local currentDir = ExtractDirectory(inputFilePath)
            return ParseShaderIncludes(currentDir, contents)
        end
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
