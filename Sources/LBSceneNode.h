#pragma once

#include "RNVector.h"
#include "RNQuaternion.h"
#include "RNMatrix.h"

namespace LB
{
	class SceneNode
	{
	public:
		const RN::Matrix &GetModelMatrix();

		inline void SetRotation(const RN::Quaternion &rotation)
		{
			_modelMatrixIsDirty = true;
			_rotation = rotation;
		}

		inline void SetScale(const RN::Vector3 &scale)
		{
			_modelMatrixIsDirty = true;
			_scale = scale;
		}

		inline void SetPosition(const RN::Vector3 &position)
		{
			_modelMatrixIsDirty = true;
			_position = position;
		}

		inline RN::Quaternion GetRotation() const
		{
			return _rotation;
		}

		inline RN::Vector3 GetScale() const
		{
			return _scale;
		}

		inline RN::Vector3 GetPosition() const
		{
			return _position;
		}

	private:
		RN::Vector3 _position;
		RN::Vector3 _scale;
		RN::Quaternion _rotation;

		bool _modelMatrixIsDirty;
		RN::Matrix _modelMatrix;
	};
}