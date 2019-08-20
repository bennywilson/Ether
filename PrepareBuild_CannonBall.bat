rd /s /q Builds\CannonBall\CannonBall\CannonBall\Assets
rd /s /q Builds\CannonBall\CannonBall\CannonBall\Levels

rd /s /q Builds\CannonBall\kbEngine\assets\Editor
rd /s /q Builds\CannonBall\kbEngine\assets\Models
rd /s /q Builds\CannonBall\kbEngine\assets\Shaders
rd /s /q Builds\CannonBall\kbEngine\assets\Textures

mkdir Builds\CannonBall\CannonBall\CannonBall\Assets
mkdir Builds\CannonBall\CannonBall\CannonBall\Levels
mkdir Builds\CannonBall\CannonBall\CannonBall\logs

mkdir Builds\CannonBall\kbEngine\assets\Editor
mkdir Builds\CannonBall\kbEngine\assets\Models
mkdir Builds\CannonBall\kbEngine\assets\Shaders
mkdir Builds\CannonBall\kbEngine\assets\Textures

robocopy CannonBall\CannonBall\levels\ Builds\CannonBall\CannonBall\CannonBall\Levels /s /e /xd cvs
robocopy CannonBall\CannonBall\Assets\ Builds\CannonBall\CannonBall\CannonBall\Assets /s /e /xd cvs

robocopy kbEngine\assets\Editor Builds\CannonBall\kbEngine\assets\Editor /s /e /xd cvs
robocopy kbEngine\assets\Models Builds\CannonBall\kbEngine\assets\Models /s /e /xd cvs
robocopy kbEngine\assets\Shaders Builds\CannonBall\kbEngine\assets\Shaders /s /e /xd cvs
robocopy kbEngine\assets\Textures Builds\CannonBall\kbEngine\assets\Textures /s /e /xd cvs

copy CannonBall\x64\Release\CannonBall.exe Builds\CannonBall\CannonBall\CannonBall\CannonBall.exe
copy kbEngine\lib\d3dx11_42.dll Builds\CannonBall\CannonBall\CannonBall\d3dx11_42.dll


pause