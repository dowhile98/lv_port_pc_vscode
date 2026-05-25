# Guía: Ejecutar el simulador en Windows con WSL2

Paso a paso para instalar WSL2 en Windows, compilar el proyecto y ejecutar el simulador gráfico.

---

## Requisitos previos

- Windows 10 Build 19041 o superior, o Windows 11
- Acceso de Administrador en Windows
- Conexión a Internet

---

## Paso 1: Instalar WSL2

Abrir **PowerShell como Administrador** y ejecutar:

```powershell
wsl --install
```

Este comando instala WSL2 con Ubuntu por defecto. Reiniciar el equipo cuando lo solicite.

Al reiniciar, Ubuntu abrirá automáticamente y pedirá crear un usuario y contraseña para el entorno Linux.

> **Windows 10 Build 19041–19042 (versiones antiguas):** si el comando anterior falla, habilitar WSL manualmente:
>
> ```powershell
> dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
> dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
> wsl --set-default-version 2
> ```
>
> Luego instalar Ubuntu desde la Microsoft Store.

---

## Paso 2: Verificar que WSL2 está activo

En PowerShell:

```powershell
wsl --list --verbose
```

La distribución instalada debe mostrar `VERSION 2`. Si aparece `1`, convertirla:

```powershell
wsl --set-version Ubuntu 2
```

---

## Paso 3: Instalar dependencias en Ubuntu (WSL)

Abrir la terminal de Ubuntu (buscar "Ubuntu" en el menú de inicio) y ejecutar:

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    libsdl2-dev \
    git
```

---

## Paso 4: Obtener el proyecto

Clonar el repositorio **dentro del filesystem de WSL** (no en `/mnt/c/`):

```bash
cd ~
git clone --recursive https://github.com/tu-org/lv_port_pc_vscode
cd lv_port_pc_vscode
```

> **Importante:** NO colocar el proyecto en `/mnt/c/Users/...` (filesystem de Windows).
> CMake no puede crear archivos temporales en NTFS desde WSL y fallará con `Operation not permitted`.
> El proyecto debe estar en el home de WSL: `~/lv_port_pc_vscode`.

Si ya tienes el proyecto descargado en Windows (por ejemplo en `C:\Users\TuUsuario\Downloads\lv_port_pc_vscode`), copiarlo al filesystem de WSL:

```bash
cp -r /mnt/c/Users/TuUsuario/Downloads/lv_port_pc_vscode ~/lv_port_pc_vscode
cd ~/lv_port_pc_vscode
```

---

## Paso 5: Compilar

Desde la raíz del proyecto:

```bash
cmake -B build -S . -G Ninja
cmake --build build
```

La primera compilación tarda varios minutos (compila LVGL + FreeRTOS + la app completa).
Las compilaciones siguientes son incrementales y mucho más rápidas.

---

## Paso 6: Ejecutar el simulador

```bash
./bin/main
```

Se abrirá una ventana gráfica en Windows mostrando la interfaz del simulador.

> WSL2 en Windows 11 (y Windows 10 con actualizaciones recientes) incluye **WSLg**, que provee el servidor gráfico automáticamente. No se requiere configuración adicional.

---

## Paso 7: Crear un atajo para ejecutar rápido

Agregar un alias en el archivo de configuración del shell:

```bash
echo 'alias sim="~/lv_port_pc_vscode/bin/main"' >> ~/.bashrc
source ~/.bashrc
```

A partir de ese momento, desde cualquier directorio en la terminal WSL ejecutar simplemente:

```bash
sim
```

---

## Recompilar tras cambios en el código

```bash
cd ~/lv_port_pc_vscode
cmake --build build
sim
```

## Compilación limpia (desde cero)

```bash
cd ~/lv_port_pc_vscode
rm -rf build
cmake -B build -S . -G Ninja
cmake --build build
sim
```

---

## Solución de problemas comunes

| Error                              | Causa                                  | Solución                                         |
| ---------------------------------- | -------------------------------------- | ------------------------------------------------ |
| `Operation not permitted` en cmake | Proyecto en `/mnt/c/` (NTFS)           | Copiar el proyecto a `~/` dentro de WSL          |
| `Could not find SDL2`              | `libsdl2-dev` no instalado             | `sudo apt install libsdl2-dev`                   |
| Ventana no aparece                 | WSLg no disponible (Win10 muy antiguo) | Instalar VcXsrv y configurar `export DISPLAY=:0` |
| `Command 'main' not found`         | Falta `./` al ejecutar                 | Usar `./bin/main`, no solo `main`                |
| Compilación lenta                  | Primera vez o limpia                   | Normal — las siguientes son incrementales        |
