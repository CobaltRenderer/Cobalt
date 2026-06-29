# Specify our source files
set(Marshalling_HEADERS
    "Marshalling.pkg"
    "CalculateDecomposedSTLContainerSize.h"
    "CalculateDecomposedSTLContainerSize.inl"
    "CalculateSTLContainerNestingDepth.h"
    "CalculateSTLContainerNestingDepth.inl"
    "ClearSTLContainer.h"
    "ClearSTLContainer.inl"
    "CreateSTLContainerItemArray.h"
    "CreateSTLContainerItemArray.inl"
    "CreateSTLContainerKeyMarshallers.h"
    "CreateSTLContainerKeyMarshallers.inl"
    "DecomposeSTLContainer.h"
    "DecomposeSTLContainer.inl"
    "DeleteSTLContainerItemArray.h"
    "DeleteSTLContainerItemArray.inl"
    "DeleteSTLContainerKeyMarshallers.h"
    "DeleteSTLContainerKeyMarshallers.inl"
    "EnsureSTLContainerKeySizesMatch.h"
    "EnsureSTLContainerKeySizesMatch.inl"
    "GetNextNestedContainerElementType.h"
    "GetLastNestedContainerElementType.h"
    "HasMarshalConstructor.h"
    "IMarshallingObject.h"
    "IMarshalSource.h"
    "IMarshalTarget.h"
    "INestedMarshaller.h"
    "INestedMarshallerBase.h"
    "IsAssignable.h"
    "IsBaseOfEmulator.h"
    "IsComplete.h"
    "IsLastNestedContainerElement.h"
    "IsNonConstRValueReference.h"
    "IsOnlyMovable.h"
    "IsSTLContainerNestedElementMarshallableOrSameSize.h"
    "IsSTLContainerNestedElementMarshallableOrSameSize.inl"
    "IsThisOrNestedElementLastNestedContainerElement.h"
    "MarshalIn.h"
    "MarshalInOut.h"
    "MarshalInOutStringOperators.inl"
    "MarshalInStringOperators.inl"
    "MarshallableDelete.h"
    "MarshalObjectHelper.h"
    "MarshalObjectTag.h"
    "MarshalOperators.h"
    "MarshalOut.h"
    "MarshalPreprocessorMacros.h"
    "MarshalRet.h"
    "MarshalRetStringOperators.inl"
    "MarshalSource.h"
    "MarshalTarget.h"
    "NestedContainerHasKeys.h"
    "NestedMarshaller.h"
    "NoBoundObjectTag.h"
    "RecomposeSTLContainer.h"
    "RecomposeSTLContainer.inl"
    "TupleHelper.h"
)

# Define our project targets
add_library(Marshalling_Lib INTERFACE)
add_library(Cobalt::Marshalling ALIAS Marshalling_Lib)
target_include_directories(Marshalling_Lib INTERFACE ${COBALT_INTERNAL_INCLUDE_DIR})

# Dummy targets to show in the IDE and register for clang-format
add_library(Marshalling STATIC ${Marshalling_HEADERS})
register_clang_format_target(Marshalling USAGE_TARGET Cobalt::Marshalling)
set_target_properties(Marshalling PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(Marshalling PROPERTIES FOLDER "Libraries/Public")

# Transfer required files to their output locations
cobalt_install_sdk_headers(Marshalling ${Marshalling_HEADERS})

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${Marshalling_HEADERS})
source_group("MarshalObjectHelper" FILES
    "MarshalObjectHelper.h"
)
source_group("MarshalSource" FILES
    "IMarshalSource.h"
    "MarshalSource.h"
)
source_group("MarshalTarget" FILES
    "IMarshalTarget.h"
    "MarshalTarget.h"
)
source_group("Marshallers" FILES
    "MarshalIn.h"
    "MarshalInOut.h"
    "MarshalOut.h"
    "MarshalRet.h"
)
source_group("Marshallers/Operators" FILES
    "MarshalInStringOperators.inl"
    "MarshalRetStringOperators.inl"
    "MarshalInOutStringOperators.inl"
)
source_group("NestedMarshaller" FILES
    "INestedMarshallerBase.h"
    "INestedMarshaller.h"
    "NestedMarshaller.h"
)
source_group("STLContainerDecomposition" FILES
    "CalculateDecomposedSTLContainerSize.h"
    "CalculateSTLContainerNestingDepth.h"
    "ClearSTLContainer.h"
    "CreateSTLContainerItemArray.h"
    "CreateSTLContainerKeyMarshallers.h"
    "DecomposeSTLContainer.h"
    "DeleteSTLContainerItemArray.h"
    "DeleteSTLContainerKeyMarshallers.h"
    "RecomposeSTLContainer.h"
    "IsSTLContainerNestedElementMarshallableOrSameSize.h"
    "TupleHelper.h"
    "EnsureSTLContainerKeySizesMatch.h"
    "CalculateDecomposedSTLContainerSize.inl"
    "CalculateSTLContainerNestingDepth.inl"
    "ClearSTLContainer.inl"
    "CreateSTLContainerItemArray.inl"
    "CreateSTLContainerKeyMarshallers.inl"
    "DecomposeSTLContainer.inl"
    "DeleteSTLContainerItemArray.inl"
    "DeleteSTLContainerKeyMarshallers.inl"
    "RecomposeSTLContainer.inl"
    "IsSTLContainerNestedElementMarshallableOrSameSize.inl"
    "EnsureSTLContainerKeySizesMatch.inl"
)
source_group("Type Traits" FILES
    "HasMarshalConstructor.h"
    "IsLastNestedContainerElement.h"
    "IsNonConstRValueReference.h"
    "IsThisOrNestedElementLastNestedContainerElement.h"
    "GetLastNestedContainerElementType.h"
    "GetNextNestedContainerElementType.h"
    "NestedContainerHasKeys.h"
    "IsBaseOfEmulator.h"
    "IsOnlyMovable.h"
    "IsAssignable.h"
    "IsComplete.h"
)
