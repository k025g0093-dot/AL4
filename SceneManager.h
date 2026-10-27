#pragma once


enum class Phese {
	kFadeIn,
	kMain,
	kDeht,
	kClear,
	kFadeOut,
};

class SceneManager {

public:

	virtual ~SceneManager()= default;


	virtual void InitIalize()=0;
	virtual void Updete()=0;
	virtual void Draw()=0;


};
