#include "stdafx.h"
#include "LBSceneNode.h"

namespace LB
{
	const RN::Matrix &SceneNode::GetModelMatrix()
	{
		if(_modelMatrixIsDirty)
		{
			_modelMatrix = RN::Matrix::WithTranslation(_position);
			_modelMatrix *= RN::Matrix::WithRotation(_rotation);
			_modelMatrix *= RN::Matrix::WithScaling(_scale);
		}

		return _modelMatrix;
	}
}
