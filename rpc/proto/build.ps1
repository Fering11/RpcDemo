# PowerShell 脚本用于编译 proto 文件

# 设置目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProtoDir = $ScriptDir
$BuildDir = Join-Path $ScriptDir "build"

# 创建 build 目录（如果不存在）
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

Write-Host "开始编译 proto 文件..." -ForegroundColor Green

# 检查 protoc 是否安装
$protocPath = Get-Command protoc -ErrorAction SilentlyContinue
if (-not $protocPath) {
    Write-Host "错误: protoc 未安装或不在 PATH 中" -ForegroundColor Red
    exit 1
}

# 遍历所有 .proto 文件
$protoFiles = Get-ChildItem -Path $ProtoDir -Filter "*.proto"
$protoCount = 0
$successCount = 0
$failCount = 0

if ($protoFiles.Count -eq 0) {
    Write-Host "未找到 proto 文件" -ForegroundColor Red
    exit 1
}

foreach ($protoFile in $protoFiles) {
    $protoCount++
    $filename = $protoFile.Name
    
    Write-Host "正在编译: $filename" -ForegroundColor Green
    
    # 编译 proto 文件，输出到 build 目录
    $process = Start-Process -FilePath "protoc" `
        -ArgumentList "--cpp_out=$BuildDir", "--proto_path=$ProtoDir", $protoFile.FullName `
        -NoNewWindow -Wait -PassThru
    
    if ($process.ExitCode -eq 0) {
        Write-Host "✓ 成功编译: $filename" -ForegroundColor Green
        $successCount++
    } else {
        Write-Host "✗ 编译失败: $filename" -ForegroundColor Red
        $failCount++
    }
    Write-Host ""
}

# 输出统计信息
Write-Host "================================" -ForegroundColor Green
Write-Host "编译完成！" -ForegroundColor Green
Write-Host "总计: $protoCount 个文件"
Write-Host "成功: $successCount" -ForegroundColor Green
if ($failCount -gt 0) {
    Write-Host "失败: $failCount" -ForegroundColor Red
}
Write-Host "生成的文件位于: $BuildDir" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green

# 如果有失败的文件，返回错误码
if ($failCount -gt 0) {
    exit 1
}

exit 0
