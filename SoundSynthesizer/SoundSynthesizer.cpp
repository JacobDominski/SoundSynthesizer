#include <iostream>
#include <list>
#include <algorithm>
using namespace std;

#define FTYPE double
#include "olcNoiseMaker.h";

namespace synth {

	FTYPE w(double dHertz) {
		return dHertz * 2.0 * PI;
	}

	struct note {
		int id;
		FTYPE on;
		FTYPE off;
		bool active;
		int channel;

		note() {
			id = 0;
			on = 0.0;
			off = 0.0;
			active = false;
			channel = 0;
		}
	};

	const int OSC_SINE = 0;
	const int OSC_SQUARE = 1;
	const int OSC_TRIANGLE = 2;
	const int OSC_SAW_ANA = 3;
	const int OSC_SAW_DIG = 4;
	const int OSC_NOISE = 5;

	FTYPE osc(const FTYPE dHertz, const FTYPE dTime, const int nType = OSC_SINE, const FTYPE dLFOHertz = 0.0, const FTYPE dLFOAmplitude = 0.0, FTYPE dCustom = 50.0) {

		FTYPE dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * (sin(w(dLFOHertz) * dTime));

		switch (nType) {

		case OSC_SINE:
			return sin(dFreq);

		case OSC_SQUARE:
			return sin(dFreq) > 0.0 ? 1.0 : -1.0;

		case OSC_TRIANGLE:
			return asin(sin(dFreq) * 2.0 / PI);

		case OSC_SAW_ANA:
		{
			FTYPE dOutput = 0.0;
			for (FTYPE n = 1.0; n < dCustom; n++) {
				dOutput += (sin(n * dFreq)) / n;
			}
			return dOutput * (2.0 / PI);
		}

		case OSC_SAW_DIG:
			return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

		case OSC_NOISE:
			return 2.0 * ((FTYPE)rand() / (FTYPE)RAND_MAX) - 1.0;

		default:
			return 0;
		}
	}


	const int SCALE_DEFAULT = 0;

	FTYPE scale(const int nNoteID, const int nScaleID = SCALE_DEFAULT) {
		switch (nScaleID) {
		case SCALE_DEFAULT: default:
			return 236 * pow(1.0594630943592952645618252949463, nNoteID);
		}
	}

	struct envelope {
		virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) = 0;
	};

	struct envelope_adsr : public envelope {
		FTYPE dAttackTime;
		FTYPE dDecayTime;
		FTYPE dSustainAmplitude;
		FTYPE dReleaseTime;
		FTYPE dStartAmplitude;

		envelope_adsr() {
			dAttackTime = 0.1;
			dDecayTime = 0.1;
			dSustainAmplitude = 1.0;
			dReleaseTime = 0.2;
			dStartAmplitude = 1.0;
			
		}

		FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) {
			
			FTYPE dAmplitude = 0.0;
			FTYPE dReleaseAmplitude = 0.0;

			

			if (dTimeOn > dTimeOff) {

				FTYPE dLifeTime = dTime - dTimeOn;

				if (dLifeTime <= dAttackTime) {
					dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
				}
				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime)) {
					dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
				}
				if (dLifeTime > (dAttackTime + dDecayTime)) {
					dAmplitude = dSustainAmplitude;
				}
			}
			else {

				FTYPE dLifeTime = dTimeOff - dTimeOn;

				if (dLifeTime <= dAttackTime) {
					dReleaseAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
				}
				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime)) {
					dReleaseAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
				}
				if (dLifeTime > (dAttackTime + dDecayTime)) {
					dReleaseAmplitude = dSustainAmplitude;
				}

				dAmplitude = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
			}

			if (dAmplitude <= 0.000) {
				dAmplitude = 0.0;
			}

			return dAmplitude;
		}
	};

	FTYPE env(const FTYPE dTime, envelope& env, const FTYPE dTimeOn, const FTYPE dTimeOff) {
		return env.amplitude(dTime, dTimeOn, dTimeOff);
	}


	struct instrument_base {
		FTYPE dVolume;
		synth::envelope_adsr env;
		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished) = 0;
	};


	struct instrument_bell : public instrument_base {
		instrument_bell() {
			env.dAttackTime = 0.01;
			env.dDecayTime = 1.0;
			
			env.dSustainAmplitude = 0.0;
			env.dReleaseTime = 1.0;

			dVolume = 1.0;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished) {
			
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;
			
			FTYPE dSound =
				+ 1.00 * synth::osc(n.on - dTime, synth::scale(n.id + 12), synth::OSC_SINE, 5.0, 0.001)
				+ 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 24))
				+ 0.25 * synth::osc(n.on - dTime, synth::scale(n.id + 36));

			return dAmplitude * dSound * dVolume;
		}
	};

	struct instrument_bell8 : public instrument_base {
		instrument_bell8() {
			env.dAttackTime = 0.01;
			env.dDecayTime = 0.5;

			env.dSustainAmplitude = 0.8;
			env.dReleaseTime = 1.0;

			dVolume = 1.0;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool& bNoteFinished) {

			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;

			FTYPE dSound =
				+1.00 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 12))
				+ 0.25 * synth::osc(n.on - dTime, synth::scale(n.id + 24));

			return dAmplitude * dSound * dVolume;
		}
	};


	struct instrument_harmonica : public instrument_base {
		instrument_harmonica() {
			env.dAttackTime = 0.05;
			env.dDecayTime = 1.0;
			env.dSustainAmplitude = 0.95;
			env.dReleaseTime = 0.1;
			dVolume = 1.0;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool& bNoteFinished) {

			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;

			FTYPE dSound =
				+1.00 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 12), synth::OSC_SQUARE)
				+ 0.05 * synth::osc(n.on - dTime, synth::scale(n.id + 24), synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}
	};

}
 

vector<synth::note> vecNotes;
mutex muxNotes;
synth::instrument_bell instBell;
synth::instrument_harmonica instHarm;

typedef bool(*lambda)(synth::note const& item);
template<class T>

void safe_remove(T& v, lambda f) {
	auto n = v.begin();
	while (n != v.end()) {
		if (!f(*n)) {
			n = v.erase(n);
		}
		else {
			++n;
		}
	}
}

FTYPE MakeNoise(int nChannel, FTYPE dTime) {

	unique_lock<mutex> lm(muxNotes);
	FTYPE dMixedOutput = 0.0;

	for (auto& n : vecNotes) {
		bool bNoteFinished = false;
		FTYPE dSound = 0;
		if (n.channel == 2) {
			dSound = instBell.sound(dTime, n, bNoteFinished);
		}
		if (n.channel == 1) {
			dSound = instHarm.sound(dTime, n, bNoteFinished) * 0.5;
			
		}
		dMixedOutput += dSound;
		if (bNoteFinished && n.off > n.on) {
			n.active = false;
		}
	}

	safe_remove<vector<synth::note>>(vecNotes, [](synth::note const& item) { return item.active; });

	return dMixedOutput * 0.2;
}

int main() {
	
	wcout << "Synthesizer 1.0" << std::endl;

	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	for (auto d : devices) {
		wcout << "Found Output Device: " << d << endl;
	}

	wcout << "Using Device: " << devices[0] << endl;

	wcout << "Controls: Left CTRL - Reset | Space - Switch Instruments" << endl;

	wcout << endl <<
		"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
		"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
		"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
		"|     |     |     |     |     |     |     |     |     |     |" << endl <<
		"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  <  |  .  |  /  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;

	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	sound.SetUserFunction(MakeNoise);

	char keyboard[129];
	memset(keyboard, ' ', 127);
	keyboard[128] = '\0';

	auto clock_old_time = chrono::high_resolution_clock::now();
	auto clock_real_time = chrono::high_resolution_clock::now();
	double dElapsedTime = 0.0;
	int nChannel = 2;
	string instrument = "Piano    ";
	bool instToggle = false;

	while (1) {

		if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
			if (!vecNotes.empty()) {
				
				for (int i = 0; i < vecNotes.size(); i++) {
					vecNotes.erase(vecNotes.begin());
				}
			}
			
		}

		if (GetAsyncKeyState(VK_SPACE) & 0x8000 && !instToggle) {
			do {
				nChannel = 1;
				instrument = "Harmonica";
				instToggle = true;
			} while (GetAsyncKeyState(VK_SPACE) & 0x8000);
			
		}
		else if (GetAsyncKeyState(VK_SPACE) & 0x8000 && instToggle) {
			do{
				nChannel = 2;
				instrument = "Piano    ";
				instToggle =false;
			} while (GetAsyncKeyState(VK_SPACE) & 0x8000);
		}
		
		for (int k = 0; k < 16; k++) {

			short nKeyState = GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k]));
			//wcout << "Key: " << nKeyState << endl;
			double dTimeNow = sound.GetTime();
			muxNotes.lock();
			auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k](synth::note const& item) { return item.id == k; });

			if (noteFound == vecNotes.end()) {
				
				if (nKeyState & 0x8000) {

					synth::note n;
					n.id = k;
					n.on = dTimeNow;
					//CHANGE INSTRUMENT
					/*
						channel 1 = Harmonica
						channel 2 = Bell -> (Piano)
					*/
					n.channel = nChannel;
					n.active = true;

					vecNotes.emplace_back(n);
				}
				else {

				}
				
			}
			else {
				if (nKeyState & 0x8000) {
					if (noteFound->off > noteFound->on) {
						noteFound->on = dTimeNow;
						noteFound->active = true;
					}
				}
				else {
					if (noteFound->off < noteFound->on) {
						noteFound->off = dTimeNow;
					}
				}
			}
			muxNotes.unlock();
		}
		cout << "\rNotes: " << vecNotes.size() << " | Intrument: " << instrument << "";
	}




	return 0;
}