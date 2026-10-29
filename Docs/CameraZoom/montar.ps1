$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$dir = $PSScriptRoot
$names = @('minimo','padrao','maximo')
$titles = @('MINIMO - 200 cm','PADRAO - 450 cm','MAXIMO - 800 cm')
$first = [System.Drawing.Image]::FromFile((Join-Path $dir 'padrao.png'))
$width = 1200
$height = [int]($first.Height * $width / $first.Width)
$first.Dispose()
$canvas = [System.Drawing.Bitmap]::new($width,(($height+42)*3))
$g = [System.Drawing.Graphics]::FromImage($canvas)
$g.Clear([System.Drawing.Color]::FromArgb(16,20,24))
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$font = [System.Drawing.Font]::new('Arial',20)
for ($idx=0;$idx -lt 3;$idx++) {
    $y=$idx*($height+42)
    $g.DrawString($titles[$idx],$font,[System.Drawing.Brushes]::White,18,($y+6))
    $img=[System.Drawing.Image]::FromFile((Join-Path $dir ($names[$idx]+'.png')))
    $g.DrawImage($img,[System.Drawing.Rectangle]::new(0,($y+42),$width,$height))
    $img.Dispose()
}
$canvas.Save((Join-Path $dir 'zoom_comparacao.png'),[System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose();$canvas.Dispose();$font.Dispose()
