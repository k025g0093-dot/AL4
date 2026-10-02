#pragma once
#include "KamataEngine.h"

struct StageData {
	std::string name;
	int32_t timeLimit;
};

class stageManager {
public:
	void LoadStageDataCsv();

	// 範囲チェック付きのゲッター
	const StageData& GetStageData(int32_t index) const {
		assert(index >= 0 && index < stageData_.size());
		return stageData_[index];
	}

	// 現在のステージを取得（参照で返す）
	const StageData& GetCurrentStageIndex() const { return GetStageData(currentStageIndex_); }

	void SetCurrentStageIndex(int32_t index) {
		// 必要であればここで範囲チェックを行う
		if (index >= 0 && index < (int32_t)stageData_.size()) {
			currentStageIndex_ = index;
		}
	}

	void SetCurrentStageIndexByName(const std::string& name);

private:
	std::vector<StageData> stageData_;
	int32_t currentStageIndex_ = 0;
};
