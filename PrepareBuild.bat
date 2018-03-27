rd /s /q Build\GameBase\GameBase\Assets
rd /s /q Build\GameBase\GameBase\Levels

rd /s /q Build\kbEngine\assets\Editor
rd /s /q Build\kbEngine\assets\Models
rd /s /q Build\kbEngine\assets\Shaders
rd /s /q Build\kbEngine\assets\Textures

mkdir Build\GameBase\GameBase\Assets
mkdir Build\GameBase\GameBase\Levels
mkdir Build\GameBase\GameBase\logs

mkdir Build\kbEngine\assets\Editor
mkdir Build\kbEngine\assets\Models
mkdir Build\kbEngine\assets\Shaders
mkdir Build\kbEngine\assets\Textures

robocopy GameBase\GameBase\levels\ Build\GameBase\GameBase\Levels /s /e /xd cvs
robocopy GameBase\GameBase\Assets\ Build\GameBase\GameBase\Assets /s /e /xd cvs

robocopy kbEngine\assets\Editor Build\kbEngine\assets\Editor /s /e /xd cvs
robocopy kbEngine\assets\Models Build\kbEngine\assets\Models /s /e /xd cvs
robocopy kbEngine\assets\Shaders Build\kbEngine\assets\Shaders /s /e /xd cvs
robocopy kbEngine\assets\Textures Build\kbEngine\assets\Textures /s /e /xd cvs

copy GameBase\x64\Release\GameBase.exe Build\GameBase\GameBase\GameBase.exe
copy kbEngine\lib\d3dx11_42.dll Build\GameBase\GameBase\d3dx11_42.dll


pause