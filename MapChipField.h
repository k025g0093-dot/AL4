#pragma once
#include "KamataEngine.h"
enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
	kPlayer,//player
	kEnemy,//盾持ち
};

enum MapChipCharIndex {
	kChipType = 0,
	kChipSubId=1
};

struct MapChipDataUnit { 
	MapChipType type;
	uint8_t subID;
};


struct MapChipData {
	std::vector<std::vector<MapChipDataUnit>> data;
};

class MapChipField {

public:

	MapChipField();
	~MapChipField();

	void ResetMapChipData();
	void LoadMapChipCsv(const std ::string& filePath);
	int GetNumBlockHorizontal() const { return kNumBlockHorizontal; };
	int GetNumBlockVirtical() const { return kNumBlockVirtical; };
	//マップチップの種類の取得
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	uint8_t GetMapChipSubIDByIndex(uint32_t xIndex, uint32_t yIndex);

	//範囲矩形
	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};

	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

	//セットインデックス
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};

	//マップ番号の取得
	IndexSet GetMapChipIndexSetByPosition(const KamataEngine::Vector3& position);

	//ポジション取得
	KamataEngine::Vector3 GetMapChipPositionByIndex(
		uint32_t xIndex, uint32_t yIndex
	);
	// 1ブロックのサイズ
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;
	// ブロックの個数
	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 45;

	MapChipData* mapChipData_;
};
