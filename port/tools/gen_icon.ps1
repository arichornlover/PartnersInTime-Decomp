# Draws the Windows launcher icon. The Android icon is the same badge drawn as
# vector XML (res/mipmap-anydpi/ic_launcher.xml and the adaptive icon), because
# the repository tracks no bitmap assets. Run this after changing the art and
# commit the regenerated .ico:
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File port\tools\gen_icon.ps1
#
# Output:
#   port\src\ui\pit_launcher.ico
#
# GDI+ is used directly so the script has no third-party drawing dependency.

[CmdletBinding()]
param(
    [string]$PortRoot = ''
)

if (-not $PortRoot) { $PortRoot = Split-Path -Parent (Split-Path -Parent $PSCommandPath) }

Add-Type -AssemblyName System.Drawing

$colBgTop    = [System.Drawing.ColorTranslator]::FromHtml('#222C5C')
$colBgBottom = [System.Drawing.ColorTranslator]::FromHtml('#090D1F')
$colGold     = [System.Drawing.ColorTranslator]::FromHtml('#F2B233')
$colGoldLit  = [System.Drawing.ColorTranslator]::FromHtml('#FFD766')
$colCoTop    = [System.Drawing.ColorTranslator]::FromHtml('#6FA8FF')
$colCoBottom = [System.Drawing.ColorTranslator]::FromHtml('#2E63C8')

# Every coordinate is on a 1024x1024 design canvas and scaled by $f, so the same
# function draws the 16px taskbar icon and the 256px Explorer icon.
function New-BadgeBitmap([int]$size)
{
    $f = [double]$size / 1024.0
    $s = [single]$size

    $bmp = New-Object System.Drawing.Bitmap($size, $size)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.PixelOffsetMode   = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.Clear([System.Drawing.Color]::Transparent)

    $margin = [single](28 * $f)
    $radius = [single](210 * $f)
    $rect = New-Object System.Drawing.RectangleF($margin, $margin, ($s - 2 * $margin), ($s - 2 * $margin))
    $d = 2 * $radius

    $badge = New-Object System.Drawing.Drawing2D.GraphicsPath
    $badge.AddArc($rect.X, $rect.Y, $d, $d, 180, 90)
    $badge.AddArc(($rect.Right - $d), $rect.Y, $d, $d, 270, 90)
    $badge.AddArc(($rect.Right - $d), ($rect.Bottom - $d), $d, $d, 0, 90)
    $badge.AddArc($rect.X, ($rect.Bottom - $d), $d, $d, 90, 90)
    $badge.CloseFigure()

    $bgBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $colBgTop, $colBgBottom, 90.0)
    $g.FillPath($bgBrush, $badge)
    $bgBrush.Dispose()

    # Edge highlight so the badge reads on a light background.
    $rim = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(90, 255, 255, 255), [single](10 * $f))
    $g.DrawPath($rim, $badge)
    $rim.Dispose()
    $badge.Dispose()

    $cx = [single]($s / 2.0)
    $cy = [single]($s / 2.0)

    # Clock ring.
    $ringR = [single](300 * $f)
    $ringPen = New-Object System.Drawing.Pen($colGold, [single](34 * $f))
    $g.DrawEllipse($ringPen, ($cx - $ringR), ($cy - $ringR), (2 * $ringR), (2 * $ringR))
    $ringPen.Dispose()

    # Four tick marks crossing the ring.
    $tickPen = New-Object System.Drawing.Pen($colGoldLit, [single](30 * $f))
    $tickPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $tickPen.EndCap   = [System.Drawing.Drawing2D.LineCap]::Round
    $tickR0 = 250 * $f
    $tickR1 = 350 * $f
    foreach ($deg in 0, 90, 180, 270) {
        $a = ($deg - 90) * [math]::PI / 180.0
        $p0 = New-Object System.Drawing.PointF(
            [single]($cx + $tickR0 * [math]::Cos($a)), [single]($cy + $tickR0 * [math]::Sin($a)))
        $p1 = New-Object System.Drawing.PointF(
            [single]($cx + $tickR1 * [math]::Cos($a)), [single]($cy + $tickR1 * [math]::Sin($a)))
        $g.DrawLine($tickPen, $p0, $p1)
    }
    $tickPen.Dispose()

    # Four-point cobalt star.
    $starOuter = [single](270 * $f)
    $starInner = [single](92 * $f)
    $starPts = New-Object 'System.Drawing.PointF[]' 8
    for ($i = 0; $i -lt 8; $i++) {
        $a = -[math]::PI / 2.0 + $i * ([math]::PI / 4.0)
        if ($i % 2 -eq 0) { $rr = $starOuter } else { $rr = $starInner }
        $starPts[$i] = New-Object System.Drawing.PointF(
            [single]($cx + $rr * [math]::Cos($a)), [single]($cy + $rr * [math]::Sin($a)))
    }
    $starRect = New-Object System.Drawing.RectangleF(
        ($cx - $starOuter), ($cy - $starOuter), (2 * $starOuter), (2 * $starOuter))
    $starPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $starPath.AddPolygon($starPts)
    $starBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($starRect, $colCoTop, $colCoBottom, 90.0)
    $g.FillPath($starBrush, $starPath)
    $starBrush.Dispose()
    $starPen = New-Object System.Drawing.Pen($colGold, [single](22 * $f))
    $starPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $g.DrawPath($starPen, $starPath)
    $starPen.Dispose()
    $starPath.Dispose()

    # Golden core.
    $dotR = [single](46 * $f)
    $dotBrush = New-Object System.Drawing.SolidBrush($colGoldLit)
    $g.FillEllipse($dotBrush, ($cx - $dotR), ($cy - $dotR), (2 * $dotR), (2 * $dotR))
    $dotBrush.Dispose()

    $g.Dispose()
    return $bmp
}

function Get-PngBytes($bmp)
{
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bytes = $ms.ToArray()
    $ms.Dispose()
    return ,$bytes
}

# --- Windows .ico ---------------------------------------------------------
# It sits beside pit_icon.rc because windres resolves the ICON statement
# relative to the resource script's own directory.
$icoPath = Join-Path $PortRoot 'src\ui\pit_launcher.ico'
$icoSizes = 16, 24, 32, 48, 64, 128, 256

$pngs = @()
foreach ($sz in $icoSizes) {
    $b = New-BadgeBitmap $sz
    $pngs += ,(Get-PngBytes $b)
    $b.Dispose()
}

$fs = [System.IO.File]::Create($icoPath)
$bw = New-Object System.IO.BinaryWriter($fs)
$bw.Write([uint16]0)
$bw.Write([uint16]1)
$bw.Write([uint16]$icoSizes.Count)
$offset = 6 + 16 * $icoSizes.Count
for ($i = 0; $i -lt $icoSizes.Count; $i++) {
    $sz = $icoSizes[$i]
    if ($sz -ge 256) { $dim = [byte]0 } else { $dim = [byte]$sz }
    $bw.Write($dim)
    $bw.Write($dim)
    $bw.Write([byte]0)
    $bw.Write([byte]0)
    $bw.Write([uint16]1)
    $bw.Write([uint16]32)
    $bw.Write([uint32]$pngs[$i].Length)
    $bw.Write([uint32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($p in $pngs) { $bw.Write($p) }
$bw.Flush()
$bw.Close()
$fs.Dispose()

Write-Host "icon: $icoPath"
