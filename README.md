# Database Schema Studio

[Italiano](README.it.md)

A desktop application for learning database design: draw a conceptual entity–relationship model, turn it into a logical schema, and generate SQL to create the tables.

## Features

- Chen and UML-like views, identifiers, weak entities, binary and recursive relationships, and simple inheritance.
- Logical tables with primary keys, foreign keys and an explanation of their origin.
- SQL generation for HSQLDB 1.8, MySQL 8.4 and MariaDB 10.11, with an editable, syntax-highlighted preview.
- Diagram notes, grid snapping, alignment, PDF export and import of supported RISE elements.
- Italian and English interface, light and dark themes, high-DPI support, examples and an illustrated Orders guide.

## Install on Windows

The repository contains the source code; to use the app, download only the Windows ZIP below.

1. Download **DbSchemaStudio-0.9.7-Qt6-Windows-x64.zip** from [Releases](https://github.com/lellomele/DbSchemaStudio/releases/latest).
2. Extract the entire archive into a folder.
3. Open **DbSchemaStudio.exe** inside that folder.

Keep the DLLs and subfolders alongside the executable. No separate Qt installation is needed to run the packaged application. The distributed build requires 64-bit Windows 10 or 11.

## Use

Start with the Orders example or choose **File → New**. Add entities and attributes, mark their identifiers and connect the entities. Press **F7** to check the conceptual model, then **Generate logical** to review and create its tables. Press **F8** to select a database engine and generate SQL.

Save your project as a `.schema` file. Use **Examples** to open other models and **Help** for the illustrated guide. Change the interface language in **Appearance and language**; the initial language is Italian. Names entered in a model are not translated.

The [user guide](GUIDA-UTENTE.md) and [illustrated Orders guide](guide/Guida-rapida-Ordini.pdf) are in Italian.

## Requirements and limits

The app prepares SQL but does not connect to a database or execute statements. Run the generated code in a client for the selected engine. The HSQLDB profile requires an existing HSQLDB 1.8 database, including an HSQLDB database in LibreOffice Base; it does not create `.odb` files and is not a Firebird profile. MySQL and MariaDB profiles can also include database creation.

Conceptual relationships are binary, including recursive ones. Use an explicit associative entity for relationship attributes. Some rules, such as requiring every order to have at least one line, need additional checks beyond primary and foreign keys. Review the mapping explanation before using the SQL. RISE import reports elements it cannot translate. Linux binaries are not provided.

## Build on Windows

Use C++17, a MinGW-w64 compiler and a matching Qt 6 installation with Widgets and PrintSupport. The packaged application uses Qt 6.11.2. From PowerShell in the source folder, adapt the SDK paths:

```powershell
.\build.ps1 -QtPrefix 'C:\Qt\mingw64' -CompilerPrefix 'C:\msys64\mingw64' -BuildDirectory '.\build'
.\package.ps1 -QtPrefix 'C:\Qt\mingw64' -CompilerPrefix 'C:\msys64\mingw64' -Executable '.\build\DbSchemaStudio.exe' -Destination '.\dist\DbSchemaStudio'
```

The scripts expect the MSYS2-style Qt directory layout. Choose an empty destination for packaging, then run the executable from the completed package. Do not copy or launch the build executable on its own. `CMakeLists.txt` is also provided for CMake 3.21 or later and Qt 6.8 or later.

## Copyright

Software didattico — © 2026 Prof. ing. Raffaele Mele. [InfoTechLab](https://infotechlab.altervista.org)

Third-party notices and licenses are included in the Windows package under `licenses`. The application icon and its source are in `assets`.
