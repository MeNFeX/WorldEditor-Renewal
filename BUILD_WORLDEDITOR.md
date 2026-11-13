# WorldEditor integrat cu clientul MartySama DX9Ex

Soluția este `Client\Metin2Client.sln`. Editorul se află în
`Client\WorldEditor`, lângă bibliotecile motorului. Dependențele sunt în
`Extern`. Folderul vechi `ClientSource\EterBase` nu participă la compilare.

## Compilare

Este necesar Visual Studio 2022 cu toolsetul C++ v143, Windows SDK și
componenta **C++ MFC for latest v143 build tools (x86 & x64)**.
Componenta MFC a fost instalată pe calculatorul actual.

Din PowerShell, în directorul proiectului:

```powershell
.\tools\build-worldeditor.ps1 -Configuration WE-Debug
.\tools\build-worldeditor.ps1 -Configuration WE-Release
```

În Visual Studio: selectează `WE-Debug` sau `WE-Release`, platforma `Win32`,
apoi setează `WorldEditor` ca Startup Project și compilează soluția.
Nu folosi configurațiile obișnuite `Debug` / `Release` pentru editor.

Configurațiile WE definesc `WORLD_EDITOR` în editor și în cele 13 biblioteci
folosite. `UserInterface`, `EterPythonLib` și `CWebBrowser` sunt excluse de la
compilarea soluției în aceste configurații. Crypto++ este compilat automat în
configurația Debug sau Release corespunzătoare.

Fișierele intermediare și simbolurile de depanare sunt în `.build`.
La fiecare compilare, executabilul este generat direct în `D:\ymir work`,
iar DLL-urile necesare sunt copiate acolo automat, inclusiv din Visual Studio:

```text
D:\ymir work\WorldEditor_WE-Debug_Win32.exe
D:\ymir work\WorldEditor_WE-Release_Win32.exe
```

Titlul ferestrei afișează versiunea 1.0, configurația, platforma și data/ora
compilării. DLL-urile originale sunt salvate o singură dată înainte de înlocuire
în `D:\ymir work\.worldeditor-backup`. Închide aplicațiile care folosesc
executabilul sau DLL-urile dacă trebuie actualizate.

Biblioteca Granny
2.11.8.0 este copiată cu numele `granny2.dll`, cerut de executabil.
`SpeedTreeRT.dll` din `D_Disc` provine din folderul local `D:\ymir work` și
este versiunea x86 verificată la pornire. Nu înlocui DLL-urile de rulare
cu versiuni Granny/DevIL din alte editoare.

## Pornire

Dublu click pe `Start-WorldEditor.cmd`, sau din PowerShell:

```powershell
.\tools\run-worldeditor.ps1
```

Implicit se lansează executabilul Release din `D:\ymir work`, cu același
director de lucru. Poate fi deschis și direct de acolo prin dublu click.
Scriptul acceptă și `-Configuration WE-Debug` sau `-DataDirectory 'cale'`.
Directorul de lucru din Visual Studio are aceeași valoare implicită și poate
fi schimbat prin proprietatea MSBuild `WorldEditorDataDirectory`. Aceasta
controlează și destinația copierii automate; scriptul de compilare o acceptă
prin parametrul `-DataDirectory`.

Resursele jocului rămân în `D:\ymir work`. Pentru editarea unei hărți reale
sunt necesare harta, proprietățile obiectelor, texturile/texture set-urile,
modelele și fișierele de mediu folosite de aceasta. Pornirea aplicației nu
înseamnă că un set complet de resurse a fost încărcat sau verificat.
Codul original păstrează unele căi absolute `D:\Ymir Work`; schimbarea
directorului de lucru nu remapează aceste căi.

## Adaptări de integrare

- Proiectele folosesc v143 și headerele/bibliotecile din `Extern`.
- Referințele WorldEditor au GUID-urile soluției clientului și folosesc
  `PRTerrainLib` și `EterLocale`.
- Editorul folosește clasa `CSpeedTreeDirectX` a motorului actual.
- Apelurile de randare DX9 au parametrul explicit pentru baza vertex-urilor.
- Randarea atributelor folosește index buffer-ul scalar din `WORLD_EDITOR`.
- Optimizarea ariilor pentru client este dezactivată doar pentru editor,
  care folosește actualizarea completă a ariilor.
- Pointerul dialogului de portal și modul inițial al documentului sunt
  inițializate explicit.

Logurile de compilare sunt `.build\WE-Debug.log` și `.build\WE-Release.log`.
Resursele utilizatorului și rezultatele compilării din `Runtime` sunt ignorate
de Git. Sursele originale din celelalte pachete MartySama nu au fost modificate.
