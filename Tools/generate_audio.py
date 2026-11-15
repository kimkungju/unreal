"""프로토타입용 효과음과 배경음을 합성해 Content/Audio 아래에 WAV로 쓴다.

녹음한 소리가 아니라 사인파와 잡음을 조합해 만든 자리 표시용 소리다.
정식 음원이 생기면 같은 이름의 어셋으로 바꿔 넣으면 코드 수정 없이 교체된다.

실행: python Tools/generate_audio.py   (numpy 필요)
"""

import os
import wave

import numpy as np

SR = 44100
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Content", "Audio")
rng = np.random.default_rng(20261115)


def t(seconds):
    return np.arange(int(SR * seconds)) / SR


def noise(seconds):
    return rng.uniform(-1.0, 1.0, int(SR * seconds))


def band(signal, low, high):
    """주파수 영역에서 low~high Hz만 남긴다. 경계는 부드럽게 깎는다."""
    spectrum = np.fft.rfft(signal)
    freqs = np.fft.rfftfreq(len(signal), 1.0 / SR)
    low_edge = np.clip((freqs - low * 0.7) / max(low * 0.3, 1.0), 0.0, 1.0)
    high_edge = np.clip((high * 1.3 - freqs) / max(high * 0.3, 1.0), 0.0, 1.0)
    return np.fft.irfft(spectrum * low_edge * high_edge, len(signal))


def sweep(seconds, start_hz, end_hz):
    """start에서 end까지 지수적으로 변하는 사인파."""
    x = t(seconds)
    freq = start_hz * (end_hz / start_hz) ** (x / seconds)
    return np.sin(2 * np.pi * np.cumsum(freq) / SR)


def decay(seconds, rate):
    return np.exp(-rate * t(seconds))


def bell(seconds):
    """가운데가 부풀었다 꺼지는 모양."""
    return np.sin(np.pi * t(seconds) / seconds) ** 2


def fade_edges(signal, seconds=0.005):
    n = int(SR * seconds)
    signal = signal.copy()
    signal[:n] *= np.linspace(0.0, 1.0, n)
    signal[-n:] *= np.linspace(1.0, 0.0, n)
    return signal


def loopable(signal, overlap_seconds):
    """끝부분을 앞부분에 겹쳐 녹여서 이음매 없이 반복되게 한다."""
    n = int(SR * overlap_seconds)
    body = signal[:-n].copy()
    tail = signal[-n:]
    ramp = np.linspace(0.0, 1.0, n)
    body[:n] = body[:n] * ramp + tail * (1.0 - ramp)
    return body


def pad(signal, seconds):
    out = np.zeros(int(SR * seconds))
    out[: len(signal)] = signal[: len(out)]
    return out


def write(name, signal, peak=0.7):
    signal = signal / max(np.max(np.abs(signal)), 1e-9) * peak
    data = (signal * 32767.0).astype(np.int16)
    path = os.path.join(OUT_DIR, name + ".wav")
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SR)
        f.writeframes(data.tobytes())
    print(f"{name}.wav  {len(data) / SR:.2f}s")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    # 마법탄 시전: 위로 치솟는 음과 짧은 바람 소리.
    d = 0.35
    cast = sweep(d, 320, 1500) * decay(d, 9) + 0.5 * sweep(d, 640, 3000) * decay(d, 14)
    cast += 0.35 * band(noise(d), 2000, 9000) * decay(d, 18)
    write("SFX_Cast", fade_edges(cast), 0.6)

    # 마법탄 명중: 낮은 충격과 잔울림.
    d = 0.45
    impact = sweep(d, 140, 45) * decay(d, 11)
    impact += 0.6 * band(noise(d), 900, 6000) * decay(d, 22)
    impact += 0.25 * np.sin(2 * np.pi * 880 * t(d)) * decay(d, 16)
    write("SFX_Impact", fade_edges(impact), 0.75)

    # 순간 가속: 바람을 가르는 소리.
    d = 0.4
    dash = band(noise(d), 300, 1500) * bell(d) + 0.7 * band(noise(d), 1500, 5000) * bell(d) ** 3
    write("SFX_Dash", fade_edges(dash), 0.6)

    # 맞음: 둔탁한 타격.
    d = 0.3
    hurt = sweep(d, 110, 40) * decay(d, 13) + 0.4 * band(noise(d), 200, 1500) * decay(d, 30)
    write("SFX_Hurt", fade_edges(hurt), 0.8)

    # 적이 흩어짐: 부서지는 알갱이 소리.
    d = 0.7
    crumble = band(noise(d), 500, 5000) * decay(d, 7)
    grains = np.zeros(int(SR * d))
    for start in rng.uniform(0.0, 0.5, 26):
        i = int(start * SR)
        length = int(SR * 0.012)
        grains[i : i + length] += rng.uniform(-1, 1, length) * np.linspace(1, 0, length)
    die = crumble * 0.6 + band(grains, 1500, 9000) * 1.2 + 0.5 * sweep(d, 380, 70) * decay(d, 8)
    write("SFX_EnemyDie", fade_edges(die), 0.7)

    # 착지와 큰 충격: 깊은 울림.
    d = 1.4
    land = sweep(d, 75, 28) * decay(d, 3.5) + 0.7 * band(noise(d), 30, 220) * decay(d, 4)
    land += 0.5 * band(noise(d), 800, 5000) * decay(d, 28)
    write("SFX_Land", fade_edges(land), 0.9)

    # 시간이 느려짐: 아래로 가라앉는 음.
    d = 0.9
    slow = sweep(d, 900, 90) * bell(d) ** 0.5 * decay(d, 1.5) + 0.3 * band(noise(d), 100, 700) * bell(d)
    write("SFX_SlowMo", fade_edges(slow), 0.6)

    # 대사 넘김: 아주 짧은 똑 소리.
    d = 0.06
    tick = np.sin(2 * np.pi * 1300 * t(d)) * decay(d, 70)
    write("SFX_Tick", fade_edges(tick, 0.002), 0.25)

    # 제목: 깊은 타격 위로 화음이 천천히 떠오른다.
    d = 5.0
    x = t(d)
    hit = sweep(d, 60, 38) * decay(d, 1.4)
    chord = sum(np.sin(2 * np.pi * f * x + p) for f, p in [(220.0, 0.0), (261.63, 1.0), (329.63, 2.0), (493.88, 3.0)])
    swell = np.clip(x / 1.5, 0, 1) * np.clip((d - x) / 2.0, 0, 1)
    title = hit + 0.22 * chord * swell
    write("SFX_Title", fade_edges(title, 0.02), 0.8)

    # 비 (반복): 독백 장면의 배경.
    d = 6.0
    rain = band(noise(d + 1.0), 2500, 12000) * 0.8 + band(noise(d + 1.0), 400, 2500) * 0.35
    drops = np.zeros(len(rain))
    for start in rng.uniform(0.0, d + 0.9, 900):
        i = int(start * SR)
        length = int(SR * 0.004)
        drops[i : i + length] += rng.uniform(-1, 1, length) * np.linspace(1, 0, length)
    rain += band(drops, 1500, 8000) * 1.5
    write("LOOP_Rain", loopable(rain, 1.0), 0.45)

    # 바람 (반복): 추락 장면.
    d = 6.0
    x = t(d + 1.0)
    gust = 0.6 + 0.4 * np.sin(2 * np.pi * 0.31 * x) * np.sin(2 * np.pi * 0.13 * x + 1.0)
    wind = band(noise(d + 1.0), 150, 900) * gust + 0.4 * band(noise(d + 1.0), 900, 3500) * gust ** 2
    write("LOOP_Wind", loopable(wind, 1.0), 0.7)

    # 땅울림 (반복): 이상현상 폭주.
    d = 4.0
    x = t(d + 1.0)
    rumble = band(noise(d + 1.0), 25, 110) + 0.5 * np.sin(2 * np.pi * 41 * x) * (0.6 + 0.4 * np.sin(2 * np.pi * 3.1 * x))
    rumble += 0.25 * band(noise(d + 1.0), 300, 1200) * (0.5 + 0.5 * np.sin(2 * np.pi * 0.7 * x))
    write("LOOP_Rumble", loopable(rumble, 1.0), 0.85)

    # 평소 배경음 (반복): 느리게 일렁이는 화음.
    d = 32.0
    x = t(d + 4.0)
    ambient = np.zeros(len(x))
    for freq, rate, phase in [(73.42, 0.05, 0.0), (110.0, 0.07, 1.3), (146.83, 0.04, 2.1), (220.0, 0.06, 0.7), (261.63, 0.03, 3.0), (329.63, 0.045, 4.2)]:
        level = 0.5 + 0.5 * np.sin(2 * np.pi * rate * x + phase)
        ambient += level * (np.sin(2 * np.pi * freq * x) + 0.5 * np.sin(2 * np.pi * freq * 1.003 * x + 1.0)) / freq ** 0.5
    ambient += 0.02 * band(noise(d + 4.0), 300, 2000)
    write("MUS_Ambient", loopable(ambient, 4.0), 0.35)

    # 전투 배경음 (반복): 낮은 맥박. 분당 120박, 8초에 16박.
    d = 8.0
    x = t(d)
    combat = 0.35 * np.sin(2 * np.pi * 55 * x) * (0.7 + 0.3 * np.sin(2 * np.pi * 0.25 * x))
    beat = 0.5
    for index in range(16):
        i = int(index * beat * SR)
        strong = index % 4 == 0
        length = 0.35
        drum = sweep(length, 95 if strong else 70, 40) * decay(length, 12 if strong else 18)
        combat[i : i + len(drum)] += drum * (1.0 if strong else 0.55)
        if index % 2 == 1:
            hat = band(noise(0.05), 5000, 12000) * decay(0.05, 80)
            combat[i : i + len(hat)] += hat * 0.18
    combat += 0.12 * np.sin(2 * np.pi * 110 * x) * (np.sin(2 * np.pi * 2.0 * x) > 0.0)
    write("MUS_Combat", combat, 0.5)


if __name__ == "__main__":
    main()
