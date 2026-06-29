// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLShaderProgram.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
IStateValueInfo::~IStateValueInfo() = default;

//----------------------------------------------------------------------------------------
ITextureBindingInfo::~ITextureBindingInfo() = default;

//----------------------------------------------------------------------------------------
// ConstantStateValueInfo methods
//----------------------------------------------------------------------------------------
template<class ValueType>
ConstantStateValueInfo<ValueType>::ConstantStateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount)
: _stateId(stateId), _value(value), _arrayIndices(arrayIndices, arrayIndices + arrayIndexCount)
{}

//----------------------------------------------------------------------------------------
template<class ValueType>
StateValueId ConstantStateValueInfo<ValueType>::GetAttributeId() const
{
	return _stateId;
}

//----------------------------------------------------------------------------------------
template<class ValueType>
size_t ConstantStateValueInfo<ValueType>::GetArrayIndexCount() const
{
	return _arrayIndices.size();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
const size_t* ConstantStateValueInfo<ValueType>::GetArrayIndices() const
{
	return _arrayIndices.data();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
void ConstantStateValueInfo<ValueType>::ApplyValue(OpenGLShaderProgram* program) const
{
	static_assert(!std::is_same_v<ValueType, ValueType>, "ConstantStateValueInfo::ApplyValue called with an unsupported type!");
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<bool>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, static_cast<GLuint>(_value));
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1i(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1i(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1i(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1f(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V1Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform1f(location, (GLfloat)_value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2i(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2i(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2i(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2ui(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2ui(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2ui(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2f(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V2Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform2f(location, (GLfloat)_value.X(), (GLfloat)_value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3i(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3i(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3i(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3ui(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3ui(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3ui(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3f(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V3Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform3f(location, (GLfloat)_value.X(), (GLfloat)_value.Y(), (GLfloat)_value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4i(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4i(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4i(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4ui(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4ui(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4ui(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4f(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<V4Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform4f(location, (GLfloat)_value.X(), (GLfloat)_value.Y(), (GLfloat)_value.Z(), (GLfloat)_value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M2Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniformMatrix2fv(location, 1, GL_FALSE, _value.data());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M3Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniformMatrix3fv(location, 1, GL_FALSE, _value.data());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M4Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, _value.data());
	}
}

//----------------------------------------------------------------------------------------
// StateValueInfo methods
//----------------------------------------------------------------------------------------
template<class ValueType>
StateValueInfo<ValueType>::StateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount)
: _stateId(stateId), _value(value), _arrayIndices(arrayIndices, arrayIndices + arrayIndexCount)
{}

//----------------------------------------------------------------------------------------
template<class ValueType>
StateValueId StateValueInfo<ValueType>::GetAttributeId() const
{
	return _stateId;
}

//----------------------------------------------------------------------------------------
template<class ValueType>
size_t StateValueInfo<ValueType>::GetArrayIndexCount() const
{
	return _arrayIndices.size();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
const size_t* StateValueInfo<ValueType>::GetArrayIndices() const
{
	return _arrayIndices.data();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
void StateValueInfo<ValueType>::ApplyValue(OpenGLShaderProgram* program) const
{
	static_assert(!std::is_same_v<ValueType, ValueType>, "StateValueInfo::ApplyValue called with an unsupported type!");
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<bool>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, static_cast<GLuint>(_value));
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1i(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1i(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1i(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1ui(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform1f(location, _value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V1Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform1f(location, (GLfloat)_value.X());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2i(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2i(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2i(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2ui(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2ui(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2ui(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform2f(location, _value.X(), _value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V2Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform2f(location, (GLfloat)_value.X(), (GLfloat)_value.Y());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3i(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3i(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3i(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3ui(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3ui(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3ui(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform3f(location, _value.X(), _value.Y(), _value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V3Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform3f(location, (GLfloat)_value.X(), (GLfloat)_value.Y(), (GLfloat)_value.Z());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4Int8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4i(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4Int16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4i(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4Int32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4i(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4UInt8>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4ui(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4UInt16>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4ui(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4UInt32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4ui(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniform4f(location, _value.X(), _value.Y(), _value.Z(), _value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<V4Float64>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		//##TODO## Use double version if "GL_ARB_gpu_shader_fp64" extension is supported
		glUniform4f(location, (GLfloat)_value.X(), (GLfloat)_value.Y(), (GLfloat)_value.Z(), (GLfloat)_value.W());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M2Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniformMatrix2fv(location, 1, GL_FALSE, _value.data());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M3Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniformMatrix3fv(location, 1, GL_FALSE, _value.data());
	}
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M4Float32>::ApplyValue(OpenGLShaderProgram* program) const
{
	GLint location;
	if (program->GetUniformLocation(_stateId, _arrayIndices.data(), _arrayIndices.size(), location))
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, _value.data());
	}
}

//----------------------------------------------------------------------------------------
// TextureBindingWithCombinedSamplerInfo methods
//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::TextureBindingWithCombinedSamplerInfo(TextureId textureId, GLenum textureTarget, TextureType* texture, SamplerType* sampler)
: _textureId(textureId), _textureTarget(textureTarget), _texture(texture), _sampler(sampler)
{}

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
TextureId TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::GetTextureId() const
{
	return _textureId;
}

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
void TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::BindTexture() const
{
	// Bind the target texture
	glActiveTexture(GL_TEXTURE0 + (int)_textureId);
	glBindTexture(_textureTarget, _texture->GetTextureNo());

	// Bind the target texture sampler
	glBindSampler((GLuint)_textureId, _sampler->GetSamplerNo());
}

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
void TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::UnbindTexture() const
{
	// Unbind the target texture
	glActiveTexture(GL_TEXTURE0 + (int)_textureId);
	glBindTexture(_textureTarget, 0);

	// Unbind the target texture sampler
	glBindSampler((GLuint)_textureId, 0);
}

} // namespace cobalt::graphics
