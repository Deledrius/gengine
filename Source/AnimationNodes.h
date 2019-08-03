//
// AnimationNodes.h
//
// Clark Kromenaker
//
// Description
//
#pragma once
#include <string>

#include "FaceController.h"
#include "Vector3.h"

class Animation;
class Audio;
class VertexAnimation;

struct AnimNode
{
	int frameNumber = 0;
	
	virtual ~AnimNode() { }
	virtual void Play(Animation* anim) = 0;
	virtual void Sample(Animation* anim, int frame) { } // Sampling support is optional. Does nothing by default.
};

struct VertexAnimNode : public AnimNode
{
	// A vertex animation to play.
	VertexAnimation* vertexAnimation = nullptr;
	
	// This is a bit confusing, and I'm not totally sure why it is structured this way.
	// In ANM file, syntax for vertex anim is <frame_num>, <act_name>, <x1>, <y1>, <z1>, <angle1>, <x2>, <y2>, <z2>, <angle2>
	// The first x/y/z/angle appear to be the offset from the model's authored center to the origin.
	// The second x/y/z/angle appear to be the desired offset from the origin.
	// In other words, to properly position an object for an animation, we do (position - offsetFromOrigin)
	Vector3 offsetFromOrigin;
	float headingFromOrigin = 0.0f;
	
	Vector3 position;
	float heading = 0.0f;
	
	void Play(Animation* anim) override;
	void Sample(Animation* anim, int frame) override;
};

struct SceneTextureAnimNode : public AnimNode
{
	std::string sceneName;
	std::string sceneModelName;
	std::string textureName;
	
	void Play(Animation* anim) override;
};

struct SceneModelVisibilityAnimNode : public AnimNode
{
	std::string sceneName;
	std::string sceneModelName;
	bool visible = false;
	
	void Play(Animation* anim) override;
};

struct ModelTextureAnimNode : public AnimNode
{
	std::string modelName;
	unsigned char meshIndex = 0;
	unsigned char submeshIndex = 0;
	std::string textureName;
	
	void Play(Animation* anim) override;
};

struct ModelVisibilityAnimNode : public AnimNode
{
	std::string modelName;
	bool visible = false;
	
	void Play(Animation* anim) override;
};

struct SoundAnimNode : public AnimNode
{
	Audio* audio = nullptr;
	
	// 0 = no sound, 100 = max volume
	int volume = 100;
	
	// If a model name is defined, play the sound at the model's position.
	// Otherwise, use the specific position defined.
	std::string modelName;
	Vector3 position;
	
	// Min and max distances for hearing the sound effect.
	float minDistance = -1.0f;
	float maxDistance = -1.0f;
	
	void Play(Animation* anim) override;
};

struct FootstepAnimNode : public AnimNode
{
	std::string actorNoun;
	
	void Play(Animation* anim) override;
};

struct FootscuffAnimNode : public AnimNode
{
	std::string actorNoun;
	
	void Play(Animation* anim) override;
};

struct PlaySoundtrackAnimNode : public AnimNode
{
	std::string soundtrackName;
	//TODO: TBS variant?
	
	void Play(Animation* anim) override;
};

struct StopSoundtrackAnimNode : public AnimNode
{
	// If empty, means "stop all soundtracks."
	std::string soundtrackName;
	
	void Play(Animation* anim) override;
};

struct CameraAnimNode : public AnimNode
{
	std::string cameraPositionName;
	
	void Play(Animation* anim) override;
};

struct FaceTexAnimNode : public AnimNode
{
	std::string actorNoun;
	std::string textureName;
	FaceElement faceElement = FaceElement::Mouth;
	
	void Play(Animation* anim) override;
};

struct UnFaceTexAnimNode : public AnimNode
{
	std::string actorNoun;
	FaceElement faceElement = FaceElement::Mouth;
	
	void Play(Animation* anim) override;
};

struct LipSyncAnimNode : public AnimNode
{
	std::string actorNoun;
	std::string mouthTextureName;
	
	void Play(Animation* anim) override;
};

struct GlanceAnimNode : public AnimNode
{
	// This node is used a grand total of ONCE in the entire game - EMLCONCENTRATE.ANM
	std::string actorNoun;
	Vector3 position;
	
	void Play(Animation* anim) override;
};

struct MoodAnimNode : public AnimNode
{
	std::string actorNoun;
	std::string moodName;
	
	void Play(Animation* anim) override;
};
