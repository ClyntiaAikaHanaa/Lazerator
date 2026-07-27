# Renders a README-safe logo: the brand mark is transparent-backed, so it reads
# poorly on GitHub's light theme. Compositing it onto the plugin's dark panel
# colour keeps it visible in both themes.
Add-Type -AssemblyName System.Drawing

$root = Split-Path $PSScriptRoot -Parent
$logo = [System.Drawing.Image]::FromFile((Join-Path $root "Installer\Logo\Lazerator.png"))
$out  = Join-Path $root "Screenshot\Logo.png"

$S = 220
$bmp = New-Object System.Drawing.Bitmap($S, $S, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic

# Rounded dark plate
$r = 34
$path = New-Object System.Drawing.Drawing2D.GraphicsPath
$path.AddArc(0, 0, $r*2, $r*2, 180, 90)
$path.AddArc($S-$r*2, 0, $r*2, $r*2, 270, 90)
$path.AddArc($S-$r*2, $S-$r*2, $r*2, $r*2, 0, 90)
$path.AddArc(0, $S-$r*2, $r*2, $r*2, 90, 90)
$path.CloseFigure()
$g.FillPath((New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(13,15,18))), $path)
$g.DrawPath((New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(38,43,51), 2)), $path)

$pad = $S * 0.20
$g.DrawImage($logo, [single]$pad, [single]$pad, [single]($S-2*$pad), [single]($S-2*$pad))

$g.Dispose()
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose(); $logo.Dispose()
Write-Host "wrote $out"
