# 게임서비스
## 리더보드 / 업적 등록, 기록에 관련된 컨트랙트이다.

|      계정     |      환경      |
| ------------ | ------------- |
| itamstoreran |      메인넷     |
| itamtestgmsv | 메인넷(테스트버전) |
| itamstoreran |      기린넷     |
| itamtestgmsv | 기린넷(테스트버전) |

```
// 리더보드를 등록한다.
// 베타버전이 아니면 전체 삭제 후 등록한다.
ACTION registboard(string appId, string boardList)
- appId: 앱 아이디
- boardList: 리더보드 리스트 (JSON stringify 문자열을 입력받는다.)

boardList 구조
[
    {
        id: string // 리더보드 아이디. uint64 자료형이지만 JS big integer 문제로 문자열로 입력받는다.
        name: string // 리더보드 이름
    }
]
```

```
// 업적을 등록한다.
// 베타버전이 아니면 전체 삭제 후 등록한다.
ACTION regachieve(string appId, string achievementList)
- appId: 앱 아이디
- achievementList: 업적 리스트 (JSON stringify 문자열을 입력받는다.)

achievementList 구조
[
    {
        id: string // 업적 아이디. uint64 자료형이지만 JS big integer 문제로 문자열로 
        name: string // 업적 이름
    }
]
```

```
// 리더보드 점수 등록
ACTION score(string appId, string boardId, string score, name owner, name ownerGroup, string nickname, string data)
- appId: 앱 아이디
- boardId: 리더보드 아이디
- score: 점수
- owner: 점수를 등록할 유저
- ownerGroup: 점수를 등록할 유저의 타입 (eos: EOS 유저, itam: eos계정이 없는 유저)
- nickname: 닉네임
- data: 점수 등록 관련 데이터
```

```
// 업적 획득
ACTION acquisition(string appId, string achieveId, name owner, name ownerGroup, string data)
- appId: 앱 아이디
- achieveId: 업적 아이디
- owner: 업적을 획득할 유저
- ownerGroup: 업적을 획득할 유저의 타입 (eos: EOS 유저, itam: eos계정이 없는 유저)
- data: 업적 획득 관련 데이터
```

```
// 업적 획득 취소
ACTION cnlachieve(string appId, string achieveId, name owner, name ownerGroup, string reason)
- appId: 앱 아이디
- achieveId: 업적 아이디
- owner: 업적을 획득한 유저
- ownerGroup: 업적을 획득한 유저의 타입 (eos: EOS 유저, itam: eos계정이 없는 유저)
- reason: 취소 사유
```

```
// 리더보드 랭킹
ACTION rank(string appId, string boardId, string ranks, string period)
- appId: 앱 아이디
- boardId: 리더보드 아이디
- ranks: 랭킹 리스트 (JSON stringify, JSON 구조 검사 X)
- period: 기간
```

```
// 게임 진행 히스토리 기록
ACTION history(string appId, name owner, name ownerGroup, string data)
- appId: 앱 아이디
- owner: 유저
- ownerGroup: 유저의 타입 (eos: EOS 유저, itam: eos계정이 없는 유저)
- data: 히스토리 데이터
```