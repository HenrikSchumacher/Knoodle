(* ::Package:: *)

$path = DirectoryName[$InputFileName];

Quiet[LibraryFunctionUnload[cLinearHomotopyCollisionTriplesAllPairs]];
ClearAll[cLinearHomotopyCollisionTriplesAllPairs];

cLinearHomotopyCollisionTriplesAllPairs::usage="cLinearHomotopyCollisionTriplesAllPairs[ t0, P0, t1, P1] computes the collision information for the linear interpolation between the cyclic polygon P0 and time t0 and the cyclic polygon P1 at time t1. Suppose that the linear homotopy is parameterized by the map f : [t0,t1] x [0,1] -> R^3. Then this routine returns the list of all triples {t,x,y} satisfying f[t,x] == f[t,y]. This version uses an inefficient all-pairs approach and is meant only for debugging purposes.";

cLinearHomotopyCollisionTriplesAllPairs := cLinearHomotopyCollisionTriplesAllPairs = Module[{lib, libname, file, d, code, name, t, $logFile},
	
	d = 3;

	name = "cLinearHomotopyCollisionTriplesAllPairs";
	
	libname = name;
	
	lib = FileNameJoin[{$path,"lib", libname<>CCompilerDriver`CCompilerDriverBase`$PlatformDLLExtension}];
	
	If[
	(*True,*)
	Not[FileExistsQ[lib]],

		Print["Compiling "<>name<>"..."];

		code = StringJoin["

#define NDEBUG

#include \"WolframLibrary.h\"

//#define TOOLS_ENABLE_PROFILER

#include \"submodules/Tensors/MMA.hpp\"
#include \"KnotTools.hpp\"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;
using namespace mma;

using Homotopy_T   = LinearHomotopy_3D<Real,Int>;
using Link_T       = Link_3D<Real,Int>;

using EContainer_T = typename Link_T::EContainer_T;
using BContainer_T = typename Link_T::BContainer_T;

EXTERN_C DLLEXPORT int "<>name<>"(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res)
{
	//Profiler::Clear(\""<>$HomeDirectory<>"\");

	Real    T_0                  = get<Real>(Args[0]);
	MTensor P_0                  = get<MTensor>(Args[1]);	
	Real    T_1                  = get<Real>(Args[2]);
	MTensor P_1                  = get<MTensor>(Args[3]);

	const Int edge_count         = dimensions(P_0)[0];

	cptr<Real   > p_0 = data<Real>(P_0);
	cptr<Real   > p_1 = data<Real>(P_1);

	Link_T L ( p_0, edge_count );

	cref<EContainer_T> E_0 = L.EdgeCoordinates();
    EContainer_T E_1 ( L.EdgeCount(), 2, 3 );

	L.template ReadVertexCoordinates<true>( p_1, E_1 );

	BContainer_T B_0 ( L.Tree().NodeCount(), 3, 2 );
	BContainer_T B_1 ( L.Tree().NodeCount(), 3, 2 );

	L.Tree().ComputeBoundingBoxes( E_0, B_0 );
	L.Tree().ComputeBoundingBoxes( E_1, B_1 );

	Homotopy_T H ( L, T_0, E_0, B_0, T_1, E_1, B_1 );
	
	H.FindCollisions_AllPairs();

	MTensorWrapper<Real> results ( { H.CollisionCount(), 3 } );

	H.WriteCollisionTriples( results.data() );

	get<MTensor>(Res) = results.Tensor();

	return LIBRARY_NO_ERROR;
}"];

		(* Invoke CreateLibrary to compile the C++ code. *)
		t = AbsoluteTiming[
			lib=CreateLibrary[
				code,
				libname,
				"Language"->"C++",
				"TargetDirectory"-> FileNameJoin[{$path,"lib"}],
				(*"ShellCommandFunction"\[Rule]Print,*)
				"ShellOutputFunction"->Print,
				Get[FileNameJoin[{$path,"BuildSettings_Accelerate.m"}]]
			]
		][[1]];
		Print["Compilation done. Time elapsed = ", t, " s.\n"];
	];

	LibraryFunctionLoad[lib,name,
		{
			Real,{Real,2,"Constant"},
			Real,{Real,2,"Constant"}
		},
		{Real,2}
		(*Integer*)
	]
];
