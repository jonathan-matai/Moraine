param
(
    [String]$jsonPath
)

$jsonData = Get-Content $jsonPath | ConvertFrom-Json
$jsonDir = (Split-Path -Path $jsonPath) + "\"

function compileShaderModule([String]$sourceFile, [String] $binaryFile)
{
    If(-not $sourceFile)
    {
        return
    }

    If(!(Test-Path $binaryFile))
    {
        glslangValidator.exe -V $sourceFile -o $binaryFile
        return
    }

    $sourceTime = [datetime](Get-Item $sourceFile).LastWriteTime
    $binaryTime = [datetime](Get-Item $binaryFile).LastWriteTime
    
    If($sourceTime -gt $binaryTime)
    {
        glslangValidator.exe -V $sourceFile -o $binaryFile
    }
}

compileShaderModule ($jsonDir + $jsonData.vertexShader.glslSource)      ($jsonDir + $jsonData.vertexShader.spirvVulkan)
compileShaderModule ($jsonDir + $jsonData.fragmentShader.glslSource)    ($jsonDir + $jsonData.fragmentShader.spirvVulkan)
compileShaderModule ($jsonDir + $jsonData.tesselationShader.glslSource) ($jsonDir + $jsonData.tesselationShader.spirvVulkan)