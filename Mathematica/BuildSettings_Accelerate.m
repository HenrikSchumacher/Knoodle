(* ::Package:: *)

Print["Reading build settings from ",$InputFileName];

{$HomebrewIncludeDirectory,$HomebrewLibraryDirectory}=
Switch[$SystemID
	,
	"MacOSX-ARM64"
	,
	{ "/opt/homebrew/include", "/opt/homebrew/lib"}
	,
	"MacOSX-x86-64"
	,
	{"/usr/local/include", "/usr/local/lib"}
	
	,_,
	{Missing[],Missing[]}
];

Switch[
    $OperatingSystem
    ,
    "MacOSX", (* Compilation settings for OS X *)
    {
        "CompileOptions" -> {
            " -Wall"
            ,"-Wextra"
            ,"-Wno-unused-parameter"
            ,"-mmacosx-version-min="<>StringSplit[Import["!sw_vers &2>1","Text"]][[4]]
            ,"-std=c++20"
            ,"-fno-math-errno"
            ,"-fenable-matrix"
            ,"-mcpu=native -mtune=native"
            ,"-framework Accelerate"
            ,"-ffast-math"
            ,"-Ofast"
            ,"-flto"
            ,"-gline-tables-only"
            ,"-pthread"
            ,"-gcolumn-info"
    (*        ,"-foptimization-record-file="<>FileNameJoin[{$HomeDirectory,"RepulsionLink_OptimizationRecord.txt"}]
            ,"-Rpass-analysis=loop-distribute"
            ,"-Rpass-analysis=loop-vectorize"
            ,"-Rpass-missed=loop-vectorize"
            ,"-Rpass=loop-vectorize"*)
            }
        ,"LinkerOptions"->{ 
            "-lfftw3"
	        (*"-lamd"*)
	        (*,"-lmetis",*)
	        }
        ,"IncludeDirectories" -> {
            ParentDirectory[DirectoryName[$InputFileName]]
            ,$HomebrewIncludeDirectory
        }
        ,"LibraryDirectories" -> {
	        $HomebrewLibraryDirectory
        }
        (*,"ShellCommandFunction" -> Print*)
        ,"ShellOutputFunction" -> Print
        ,"Language"->"C++"
    }
    
    ,
    "Unix" (* Compilation settings for Linux *)
    ,
    {
        "CompileOptions" -> {" -Ofast"," -Wall -Wextra -Wno-unused-parameter -std=c++20 -march=native"}
        ,"LinkerOptions"->{"-lpthread","-lm","-ldl"}
        ,"IncludeDirectories" -> {
            ParentDirectory[DirectoryName[$InputFileName]]
        }
        ,"LibraryDirectories" -> {}
        ,"ShellOutputFunction" -> Print
        ,"Language"->"C++"
    }
    
    ,
    "Windows"(* Compilation settings for Windows *)
    ,
    {
        "CompileOptions" -> {"/EHsc", "/wd4244", "/DNOMINMAX", "/arch:AVX", "/Ot", "/std:c++20"}
        ,"LinkerOptions"->{}
        ,"IncludeDirectories" -> {
			FileNameJoin[{ParentDirectory[DirectoryName[$InputFileName]],"Knoodle"}]
			(*,FileNameJoin[{ParentDirectory[DirectoryName[$InputFileName]],"Knoodle","submodules","Tensors"}]*)
        }
        ,"LibraryDirectories" -> {FileNameJoin[{$InstallationDirectory,"SystemFiles","Libraries",$SystemID}]}
        ,"ShellOutputFunction" -> Print
        ,"Language"->"C++"
    }
]
