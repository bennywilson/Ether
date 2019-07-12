rd /s /q Builds\GameBase\GameBase\Assets
rd /s /q Builds\GameBase\GameBase\Levels

rd /s /q Builds\kbEngine\assets\Editor
rd /s /q Builds\kbEngine\assets\Models
rd /s /q Builds\kbEngine\assets\Shaders
rd /s /q Builds\kbEngine\assets\Textures

mkdir Builds\GameBase\GameBase\Assets
mkdir Builds\GameBase\GameBase\Le5vels
mkdir Builds\GameBase\GameBase\logs

mkdir Builds\kbEngine\assets\Editor
mkdir Builds\kbEngine\assets\Models
mkdir Builds\kbEngine\assets\Shaders
mkdir Builds\kbEngine\assets%\Textures

robocopy GameBase\GameBase\levels\ Builds\GameBase\GameBase\Levels /s /e /xd cvs
robocopy GameBase\GameBase\Assets\ Builds\GameBase\GameBase\Assets /s /e /xd cvs

robocopy kbEngine\assets\Editor Builds\kbEngine\assets\Editor /s /e /xd cvs
robocopy kbEngine\assets\Models Builds\kbEngine\assets\Models /s /e /xd cvs
robocopy kbEngine\assets\Shaders Builds\kbEngine\assets\Shaders /s /e /xd cvs
robocopy kbEngine\assets\Textures Builds\kbEngine\assets\Textures /s /e /xd cvs

copy GameBase\x64\Release\GameBase.exe Builds\GameBase\GameBase\GameBase.exe
copy kbEngine\lib\d3dx11_42.dll Builds\GameBase\GameBase\d3dx11_42.dll


pause