@ECHO OFF
ECHO Creating Virtual Environment...
py -3.11 -m venv ../venv
call ..\venv\Scripts\activate.bat
python -m pip install --upgrade pip
ECHO Virtual Environment created.
ECHO Installing modules from requirements.txt
python -m pip install pip-tools
pip-sync requirements.txt dev-requirements.txt
ECHO Modules installed.
