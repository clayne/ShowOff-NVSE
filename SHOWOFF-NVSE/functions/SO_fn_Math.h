﻿#pragma once
#include <armadillo>
#include <cfloat>
#include <optional>
#include "NiTypes.h"





#if _DEBUG


NVSEArrayVar* QuatToArray(NiQuaternion const& quat, Script* callingScript)
{
	ArrayElementR elems[] = { { quat.w }, { quat.x }, { quat.y }, { quat.z } };
	return g_arrInterface->CreateArray(elems, 4, callingScript);
}

// Extracted 2D numbers, for use as matrices, quaternions, etc.
template <typename T>
struct array_2d
{
	std::vector<std::vector<T>> numbers;

	//Returns num_rows (first), then num_cols
	[[nodiscard]] std::pair<size_t, size_t> GetDimensions() const
	{
		auto const num_rows = numbers.size();
		size_t const num_cols = 0;
		if (num_rows > 0)
		{
			num_cols = numbers[0].size();
		}
		return { num_rows, num_cols };
	}

	[[nodiscard]] bool IsEmpty() const { return numbers.empty(); }

	NVSEArrayVar* CreateArray(Script* callingScript)
	{
		if (IsEmpty())
			return nullptr;
		
		std::vector<ArrayElementR> elems;

		//Make a sub-array for each row.
		//Inspired by https://thispointer.com/how-to-print-two-dimensional-2d-vector-in-c/
		std::for_each(numbers.begin(),numbers.end(),[&](const auto& row) {
			std::vector<ArrayElementR> rowElems;
			std::for_each(row.begin(), row.end(),[&rowElems](const auto& elem) {
				rowElems.emplace_back(elem);
			});
			elems.emplace_back(g_arrInterface->CreateArray(rowElems.data(), rowElems.size(), callingScript));
		});
		return g_arrInterface->CreateArray(elems.data(), elems.size(), callingScript);
	}
};




// Ignores maxCol/Rows check if == 0
// Assumes array is packed
template <typename T>
std::optional<array_2d<T>> TryGetArrayNumbers(NVSEArrayVar *arr, size_t const maxCols = 0, size_t const minCols = 0, size_t const maxRows = 0, size_t const minRows = 0)
{
	if (!arr) return {};
	ArrayData const arrData(arr, true);	//assume isPacked
	if (arrData.size <= 0)
		return {};

	array_2d<T> matData;
	if (arrData.vals[0].GetType() == NVSEArrayVarInterface::kType_Array)	//is2D array
	{
		if (maxRows != 0 && arrData.size > maxRows)
			return {};
		matData.num_rows = arrData.size;
		for (int i = 0; i < arrData.size; i++)	//assume each elem is an array.
		{
			ArrayData const innerArrData(arrData.vals[i].Array(), true);
			if (innerArrData.size <= 0 || (maxCols != 0 && innerArrData.size > maxCols))
				return {};

			if (!matData.num_cols)
				matData.num_cols = innerArrData.size;
			else if (matData.num_cols != innerArrData.size)	// inner arrays must have matching sizes.
				return {};

			//Get row data from internal array.
			for (int k = 0; k < innerArrData.size; k++)	//assume each elem is a number.
			{
				matData.numbers.emplace_back(innerArrData.vals[k].Number());
			}
		}
	}
	else //1D array, only a single row (each element is a column).
	{
		matData.num_rows = 1;
		if (maxCols != 0 && arrData.size > maxCols)
			return {};
		matData.num_cols = arrData.size;
		for (int i = 0; i < arrData.size; i++)
		{
			matData.numbers.emplace_back(arrData.vals[i].Number());
		}
	}

	if (matData.num_rows > minRows || matData.num_clos > minCols)
		return {};
	return matData;
}

std::optional<NiMatrix33> Get3x3MatrixFromArray(NVSEArrayVar* arr)
{
	if (auto matData = TryGetArrayNumbers<float>(arr, 3, 3, 3, 3))
	{
		NiMatrix33 mat3x3(matData.value().numbers.data());
		return mat3x3;
	}
	return {};
}

std::optional<NiQuaternion> GetQuatFromArray(NVSEArrayVar* arr)
{
	if (auto quatData = TryGetArrayNumbers<float>(arr, 4, 4, 1, 1))
	{
		NiQuaternion quat(quatData.value().numbers.data());
		return quat;
	}
	return {};
}

//TODO: use array_2d instead of having copied code
std::optional<arma::Mat<double>> GetMatrixFromArray(NVSEArrayVar* arr)
{
	if (!arr) return {};
	ArrayData const arrData(arr, true);	//assume isPacked
	if (arrData.size <= 0)
		return {};
	
	arma::Mat<double> matrix;
	if (bool const is2D = arrData.vals[0].GetType() == NVSEArrayVarInterface::kType_Array;
		is2D == true)
	{
		int n_cols = -1;
		for (int i = 0; i < arrData.size; i++)	//assume each elem is an array.
		{
			ArrayData const innerArrData(arrData.vals[i].Array(), true);
			if (innerArrData.size <= 0)
				return {};
			
			if (n_cols == -1)
				n_cols = innerArrData.size;
			else if (n_cols != innerArrData.size)	// if inner arrays don't have matching sizes.
				return {};
			
			//Populate current row with column data contained in internal (row) array.
			arma::Row<double> NthRow(innerArrData.size);
			for (int k = 0; k < innerArrData.size; k++)	//assume each elem is a number.
			{
				NthRow(k) = innerArrData.vals[k].Number();
			}
			matrix.insert_rows(i, NthRow);
		}
	}
	else //1D array, which is only a single row (each element is a column).
	{
		arma::Row<double> row(arrData.size);
		for (int i = 0; i < arrData.size; i++)
		{
			row(i) = arrData.vals[i].Number();
		}
		matrix = row;
	}
	return matrix;
}

//todo: swap for generic version (CreateArrayFromNumbers)
NVSEArrayVar* GetMatrixAsArray(arma::Mat<double>& matrix, Script* callingScript)
{
	if (matrix.empty())
		return nullptr;
	
	NVSEArrayVar* resArr = g_arrInterface->CreateArray(nullptr, 0, callingScript);
	if (matrix.is_rowvec())	// create 1D array
	{
		auto const iter_end = matrix.end_row(0);
		for (auto iter = matrix.begin_row(0); iter != iter_end; iter++)
		{
			g_arrInterface->AppendElement(resArr, ArrayElementR(*iter));
		}
	}
	else //create 2D array (starting with rows as internal arrays)
	{
		auto CreateInternalArray = [&](arma::rowvec& row) {
			NVSEArrayVar* innerArr = g_arrInterface->CreateArray(nullptr, 0, callingScript);
			row.for_each([&innerArr](arma::mat::elem_type& val) { g_arrInterface->AppendElement(innerArr, ArrayElementR(val)); });
			g_arrInterface->AppendElement(resArr, ArrayElementR(innerArr));
		};
		matrix.each_row(CreateInternalArray);
	}
	return resArr;
}

DEFINE_COMMAND_ALT_PLUGIN(Matrix_MultiplyByMatrix, Mat_MultByMat, "Returns the matrix multiplication result of two matrix arrays.", false, kParams_TwoArrayIDs);
bool Cmd_Matrix_MultiplyByMatrix_Execute(COMMAND_ARGS)
{
	*result = 0;	// resulting matrix
	UInt32 arrA_ID, arrB_ID;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrA_ID, &arrB_ID))
		return true;
	auto matrixA = GetMatrixFromArray(g_arrInterface->LookupArrayByID(arrA_ID));
	auto matrixB = GetMatrixFromArray(g_arrInterface->LookupArrayByID(arrB_ID));
	if (matrixA && matrixB)
	{
		try{
			arma::Mat<double> resMatrix = (*matrixA) * (*matrixB);
			if (auto const matrixAsArray = GetMatrixAsArray(resMatrix, scriptObj))
				g_arrInterface->AssignCommandResult(matrixAsArray, result);
		}
		catch (std::logic_error&err){	//invalid matrix sizes for multiplication
			return true;
		}
	}
	return true;
}

DEFINE_COMMAND_PLUGIN(Matrix_AddMatrix, "Returns the addition of two matrices with equal # of rows and columns.", false, kParams_TwoArrayIDs);
bool Cmd_Matrix_AddMatrix_Execute(COMMAND_ARGS)
{
	*result = 0;	// resulting matrix
	UInt32 arrA_ID, arrB_ID;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrA_ID, &arrB_ID))
		return true;
	auto matrixA = GetMatrixFromArray(g_arrInterface->LookupArrayByID(arrA_ID));
	auto matrixB = GetMatrixFromArray(g_arrInterface->LookupArrayByID(arrB_ID));
	if (matrixA && matrixB){
		try {
			arma::Mat<double> resMatrix = (*matrixA) + (*matrixB);
			if (auto const matrixAsArray = GetMatrixAsArray(resMatrix, scriptObj))
				g_arrInterface->AssignCommandResult(matrixAsArray, result);
		}
		catch (std::logic_error& err) {	//invalid matrix sizes for addition
			return true;
		}
	}
	return true;
}

DEFINE_COMMAND_ALT_PLUGIN(Matrix_MultiplyByScalar, Mat_MultByScal, "Returns the matrix multiplied by a scalar.", false, kParams_OneArrayID_OneDouble);
bool Cmd_Matrix_MultiplyByScalar_Execute(COMMAND_ARGS)
{
	*result = 0;	// resulting matrix
	UInt32 arrID;
	double scalar;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrID, &scalar))
		return true;
	if (auto matrix = GetMatrixFromArray(g_arrInterface->LookupArrayByID(arrID)))
	{
		arma::Mat<double> resMatrix = matrix.value() * scalar;
		if (auto const matrixAsArray = GetMatrixAsArray(resMatrix, scriptObj))
			g_arrInterface->AssignCommandResult(matrixAsArray, result);
	}
	return true;
}

DEFINE_COMMAND_PLUGIN(Matrix_Transpose, "Returns the transpose of the array matrix.", false, kParams_OneArrayID);
bool Cmd_Matrix_Transpose_Execute(COMMAND_ARGS)
{
	*result = 0;	// transposeArr
	UInt32 arrID;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrID))
		return true;
	if (auto const matrix = GetMatrixFromArray(g_arrInterface->LookupArrayByID(arrID)))
	{
		arma::Mat<double> transpose = matrix.value().t();
		g_arrInterface->AssignCommandResult(GetMatrixAsArray(transpose, scriptObj), result);
	}
	return true;
}

DEFINE_COMMAND_PLUGIN(Matrix_IsMatrix, "Checks if an array is convertible to a matrix.", false, kParams_OneArrayID);
bool Cmd_Matrix_IsMatrix_Execute(COMMAND_ARGS)
{
	*result = false;	// isMatrix (bool)
	UInt32 arrID;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrID))
		return true;
	auto const arrA = g_arrInterface->LookupArrayByID(arrID);
	if (g_arrInterface->GetContainerType(arrA) == NVSEArrayVarInterface::kArrType_Array)
	{
		if (auto const matrix = GetMatrixFromArray(arrA))
			*result = true;
	}
	return true;
}

DEFINE_COMMAND_ALT_PLUGIN(Matrix3x3_GetQuaternion, Mat_GetQuat, "Returns a quaternion from a 3x3 matrix.", false, kParams_OneArrayID);
bool Cmd_Matrix3x3_GetQuaternion_Execute(COMMAND_ARGS)
{
	*result = 0;	// resulting matrix
	UInt32 arrID;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrID))
		return true;
	if (auto matrix = Get3x3MatrixFromArray(g_arrInterface->LookupArrayByID(arrID)))
	{
		const NiQuaternion quat { *matrix };
		g_arrInterface->AssignCommandResult(QuatToArray(quat, scriptObj), result);
	}
	return true;
}

DEFINE_COMMAND_ALT_PLUGIN(Quaternion_GetMatrix, Quat_GetMat, "Returns a 3x3 rotation matrix from a quaternion array.", false, kParams_OneArrayID);
bool Cmd_Quaternion_GetMatrix_Execute(COMMAND_ARGS)
{
	*result = 0;	// resulting matrix
	UInt32 arrID;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrID))
		return true;
	if (auto quat = GetQuatFromArray(g_arrInterface->LookupArrayByID(arrID)))
	{
		const NiMatrix33 mat{ *quat };
		g_arrInterface->AssignCommandResult(Matrix3x3ToArray(mat, scriptObj), result);
	}
	return true;
}



DEFINE_COMMAND_PLUGIN(Matrix_Dump, "Dumps the matrix array in console, in matrix notation.", false, kParams_OneArrayID);
bool Cmd_Matrix_Dump_Execute(COMMAND_ARGS)
{
	UInt32 arrID;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &arrID))
		return true;
	auto const arrA = g_arrInterface->LookupArrayByID(arrID);
	if (auto const matrix = GetMatrixFromArray(arrA))
	{
		Console_Print("** Dumping Array %u as Matrix **", arrID);
		std::stringstream stream;
		stream << (*matrix);
		Console_Print_Long(stream.str());
	}
	else
	{
		Console_Print("Matrix_Dump >> Array %u is not a valid Matrix.", arrID);
	}
	return true;
}

DEFINE_COMMAND_PLUGIN(TestMatrix, "debug matrix function", false, NULL);
bool Cmd_TestMatrix_Execute(COMMAND_ARGS)
{
	// Initialize the random generator
	arma::arma_rng::set_seed_random();

	std::stringstream stream;

	// Create a 4x4 random matrix and print it on the screen
	arma::Mat<double> A = arma::randu(4, 4);
	stream << "A:\n" << A << "\n";

	// Multiply A with his transpose:
	stream << "A * A.t() =\n";
	stream << A * A.t() << "\n";

	// Access/Modify rows and columns from the array:
	A.row(0) = A.row(1) + A.row(3);
	A.col(3).zeros();
	stream << "add rows 1 and 3, store result in row 0, also fill 4th column with zeros:\n";
	stream << "A:\n" << A << "\n";

	arma::Mat<double>B = arma::diagmat(A);
	stream << "B:\n" << B << "\n";

	Console_Print_Long(stream.str());

	// Save matrices A and B:
	A.save("A_mat.txt", arma::arma_ascii);
	B.save("B_mat.txt", arma::arma_ascii);

	return true;
}

/*When comparing two float values for equality, due to internal conversions between singleand double precision,
 *it's better to check if the absolute difference is less than epsilon (0.0001)
 *
 *(The Double Precision Fix from JIP LN) alleviates the problem with operator ==,
 *but doesn't eliminate it entirely. Checking absolute difference against an epsilon is far more computationally expensive than a simple comparison, obviously
 *-Jazzisparis */
DEFINE_COMMAND_ALT_PLUGIN(Flt_Equals, Float_Equals, "Returns true if the absolute difference between two floats is less than epsilon (0.0001)", false, kParams_TwoDoubles);
bool Cmd_Flt_Equals_Execute(COMMAND_ARGS)
{
	double a, b;
	*result = false;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &a, &b))
		return true;
	if (abs(a - b) < FLT_EPSILON)
		*result = true;
	return true;
}

#endif
