# 이야기 초안: 1장까지

모든 대사와 설정은 초안이다. 대사는 `Source/Isekai/Story/IsekaiStoryDirector.cpp`에 모여 있다.

## 한 줄

비 오는 밤 사고를 당한 현대인이 이세계의 하늘에서 떨어지고, 그를 주운 측량사 세라의 집에서 마력을 다루는 법을 배운다.
그러나 강해질수록 그의 마력은 짐승을 불러들이고, 그는 마을을 지키기 위해, 그리고 자신을 부른 "틈"의 정체를 알기 위해 북쪽으로 떠난다.

## 인물

| 이름 | 설정 |
|---|---|
| 나 (주인공) | 20대 초반 현대인. 비 오는 밤 횡단보도에서 사고. 이 세계에 다시 태어난 것이 아니라 그 모습 그대로 떨어졌다 |
| 세라 | 20대 후반. 이상현상 "틈"을 재고 기록하는 측량사. 무뚝뚝하고 칭찬 대신 "기록"을 한다. 마을 서쪽 끝에 혼자 산다 |
| 촌장 | 마을의 어른. 주인공을 미워하지 않지만 마을을 지켜야 하는 입장 |

## 세계의 규칙 (이야기에 쓰는 것만)

- **틈**: 하늘이나 땅에 생기는 균열. 주변의 중력이 뒤틀리고 돌이 떠오른다. 주인공은 틈을 통해 떨어졌다.
- **틈을 건너온 자**: 마력의 그릇이 크지만 닫는 법을 몰라 계속 샌다. 다루지 못하면 몸이 상한다. → **수련하는 이유**
- **샌 마력은 짐승을 부른다**: 강해질수록 더 멀리까지 "들린다". → **떠나는 이유**
- **북쪽의 탑**: 틈을 처음 기록한 곳. 틈을 닫는 법과, 건너온 자가 돌아갈 길의 단서가 있을지 모른다.

## 떠나는 이유 (개연성)

한 가지 이유만으로는 약하다. 세 겹으로 겹쳤다.

| 층 | 내용 | 어디서 드러나나 |
|---|---|---|
| 밀어내는 힘 | 수련으로 커진 마력이 짐승을 부른다. 남으면 마을이 다친다 | 모닥불 대화의 복선 → 습격 → 세라의 설명 |
| 끌어당기는 힘 | 폐허 제단이 북쪽을 가리킨다. 틈을 닫는 법, 돌아갈 길의 단서 | 습격 뒤 세라의 말 → 제단에서 직접 확인 |
| 함께 가는 이유 | 세라는 원래 틈을 재는 측량사다. 목적이 같다 | 세라의 첫 자기소개, 출발 결정 |

핵심은 **성장에 대가가 붙어 있다**는 점이다. 수련은 살기 위해 필요했지만, 수련할수록 마을이 위험해진다.
그래서 떠나는 결정이 바깥에서 갑자기 떨어진 사건이 아니라, 플레이어가 직접 한 수련의 결과가 된다.

## 흐름 (약 10분)

| 순서 | 장면 | 조작 | 새로 열리는 것 |
|---|---|---|---|
| 1 | 그림 컷 4장: 비 오는 밤 → 헤드라이트 → 낯선 하늘에서 추락 → 세라가 발견 | E로 넘기기 | |
| 2 | 세라의 집 침실에서 깨어남. 천장을 보다 세라에게 고개를 돌린다 | 대사 | |
| 3 | 출신을 묻는 선택: 사실대로 / 숨긴다 | 숫자 키 | |
| 4 | 마력이 새고 있다는 말. 마당으로 나간다 | 이동 | |
| 5 | 수련 1: 수정 셋 깨기 | 좌클릭 | 마법탄, 최대 마나 120 |
| 6 | "보름 뒤". 수련 2: 연습 상대의 공격을 세 번 피하고 쓰러뜨리기 | 우클릭 | 가속, 최대 체력 120 |
| 7 | "한 달 뒤, 밤". 수련 3: 빛을 띄운 채 화로 셋에 불 옮기기 | F, E | 빛, 최대 마나 150 |
| 8 | 모닥불 대화. "짐승들이 무언가를 듣고 찾아오는 것 같다" | 대사 | |
| 9 | 습격: 짐승 넷(하나는 원거리) → 큰 짐승 | 전투 | |
| 10 | 촌장과 세라. 짐승을 부른 것이 주인공의 마력이었음이 드러난다 | 대사 | |
| 11 | 떠나는 이유 선택: 마을을 위해 / 돌아갈 길을 찾으려고 | 숫자 키 | |
| 12 | 그림 컷 1장: 새벽의 출발. 제목 "1장 · 북쪽으로" | | |
| 13 | 세라를 따라 숲과 고갯길을 지나 동쪽 폐허로. 고갯길에서 매복 | 이동, 전투 | |
| 14 | 폐허의 제단. 북쪽을 가리키는 문양. 끝 | E | |

두 번의 선택은 기록으로 남고(`ToldTruthToSera`/`HidOriginFromSera`, `LeftForVillage`/`LeftToFindWayHome`),
습격 뒤 세라의 말, 출발 그림의 자막, 마지막 세라의 말이 그에 따라 달라진다.

## 그림 컷

`Content/Story` 폴더에 아래 이름의 PNG(또는 JPG)를 넣으면 Play를 누를 때 자동으로 가져와 쓴다.
그림이 없으면 그 자리에 설명 문구가 대신 뜬다. 16:9, 1920x1080 이상을 권한다. 화면을 꽉 채우도록 잘라 맞추고
천천히 밀거나 당기므로(켄 번스 효과) 가장자리 10% 정도는 잘려도 괜찮게 구도를 잡는다.

인물이 컷마다 달라 보이지 않게, 세라와 주인공의 설정 그림을 먼저 하나씩 만든 뒤 그 그림을 참고 이미지로 붙여 각 컷을 만드는 편이 좋다.

공통 문구 (각 프롬프트 뒤에 붙인다):

> stylized anime illustration, painterly cel shading, cinematic composition, 16:9, soft volumetric light, high detail background, no text, no logo

인물 설정:

- 주인공: young adult man, early 20s, short black hair, dark navy hoodie and jeans, sneakers
- 세라: woman in her late 20s, ash-silver hair tied in a low ponytail, sharp calm eyes, long dark teal travel cloak, leather belt with small brass surveying instruments

| 파일 | 장면 | 프롬프트 |
|---|---|---|
| Cut_01 | 비 오는 밤의 횡단보도 | A rainy night city crosswalk seen from behind a lone young man walking without an umbrella, wet asphalt reflecting red and green traffic lights, blurred city lights, heavy rain, lonely mood |
| Cut_02 | 헤드라이트 | Blinding white truck headlights filling the frame on a rainy night street, raindrops frozen in the light, the silhouette of a young man raising his arm, overexposed, dramatic |
| Cut_03 | 낯선 하늘에서 추락 | A young man in a navy hoodie falling through a vast fantasy sky above the clouds, below him ancient stone ruins with floating rock fragments, distant mountain range, two pale moons in a daytime sky, sense of vertigo and wonder |
| Cut_04 | 세라가 발견 | Inside broken stone ruins at morning, a young man lying unconscious among fallen pillars, a woman with ash-silver hair and a dark teal cloak kneeling and leaning over him, warm sunlight from behind her, floating pebbles in the air |
| Cut_05 | 새벽의 출발 | Dawn mist at a wooden village gate, two figures seen from behind (a young man in a hoodie with a travel bag and a woman in a dark teal cloak), a winding path leading east toward a mountain pass, faint floating rocks above the distant ridge, hopeful quiet mood |

이 프롬프트들은 특정 게임이나 작품의 이름을 쓰지 않았다. 생성한 그림을 출시작에 쓸 때는 쓰는 도구의 상업적 이용 조건을 확인한다.
