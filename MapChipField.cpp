#include "MapChipField.h"
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>

// コンストラクタ（ここでメモリを確保する！）
MapChipField::MapChipField() { mapChipData_ = new MapChipData(); }

// デストラクタ（ここで後始末をする！）
MapChipField::~MapChipField() { delete mapChipData_; }

// 無名のネームスペースを使用して衝突を避ける
using namespace KamataEngine;

namespace {

std::map<char, MapChipType> mapChipTable = {
    {'G', MapChipType::kBlank},
    {'B', MapChipType::kBlock},
    {'P', MapChipType::kPlayer},
    {'E', MapChipType::kEnemy},

};


} // namespace

void MapChipField::ResetMapChipData() {
	mapChipData_->data.clear();
	mapChipData_->data.resize(kNumBlockVirtical);
	for (std::vector<MapChipDataUnit>& mapChipDataLine : mapChipData_->data) {
		mapChipDataLine.resize(kNumBlockHorizontal);
		// ★追加：全て kBlank で初期化
		for (MapChipDataUnit& unit : mapChipDataLine) {
			unit.type = MapChipType::kBlank;
			unit.subID = 0;
		}
	}
}

void MapChipField::LoadMapChipCsv(const std ::string& filePath) {

	// マップチップのデータリセット
	ResetMapChipData();

	// ファイルを開く
	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	// マップチップのCSV
	std::stringstream mapChipCsv;
	// ファイルの内容を文字列ストリームにコピー
	mapChipCsv << file.rdbuf();
	// ファイルを閉じる
	file.close();

	// CSVからマップチップデータを読み込む
	for (uint32_t i = 0; i < kNumBlockVirtical; i++) {
		std::string line;
		getline(mapChipCsv, line);

		// 一行分の文字列をストリーム変換して解析しやすく
		std::istringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {


			std::string word;
			std::getline(lineStream, word, ','); // カンマ区切りでの読み込み
			word.erase(std::remove(word.begin(), word.end(), '\r'), word.end());

			if (word.empty()) {
				continue;
			}

			if (!mapChipTable.contains(word[kChipType])) {
				continue;
			}

			mapChipData_->data[i][j].type = mapChipTable[word[kChipType]];

			if (word.size() <= kChipSubId) {
				continue;
			}

			mapChipData_->data[i][j].subID = static_cast<uint8_t>(word[kChipSubId] - '0');

		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex >= kNumBlockHorizontal) { // ★ < 0を削除、>= に
		return MapChipType::kBlank;
	}
	if (yIndex >= kNumBlockVirtical) { // ★ < 0を削除、>= に
		return MapChipType::kBlank;
	}

	return mapChipData_->data[yIndex][xIndex].type;
}

uint8_t MapChipField::GetMapChipSubIDByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex >= kNumBlockHorizontal) { // ★ 修正
		return 0;
	}
	if (yIndex >= kNumBlockVirtical) { // ★ 修正
		return 0;
	}

	return mapChipData_->data[yIndex][xIndex].subID;
}

Vector3 MapChipField::GetMapChipPositionByIndex(
	uint32_t xIndex, uint32_t yIndex) {
	return Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVirtical - 1 - yIndex), 0); 
}

MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const KamataEngine::Vector3& position) {
	IndexSet indexSet{};
	indexSet.xIndex = static_cast<uint32_t>((position.x + kBlockWidth / 2) / kBlockWidth);

	indexSet.yIndex = static_cast<uint32_t>(kNumBlockVirtical - 1 - (uint32_t)(position.y / kBlockHeight));
	return indexSet;
}

MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
	// ブロックの基準位置を取得
	KamataEngine::Vector3 pos = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left = pos.x - kBlockWidth / 2.0f;
	rect.right = pos.x + kBlockWidth / 2.0f;
	rect.bottom = pos.y - kBlockHeight / 2.0f;
	rect.top = pos.y + kBlockHeight / 2.0f;

	return rect;
}