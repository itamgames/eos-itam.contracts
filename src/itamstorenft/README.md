# NFT

|      계정     |      환경      |
| ------------ | ------------- |
| itamstorenft |      메인넷     |
| itamtestsnft | 메인넷(테스트버전) |
| itamstorenft |      기린넷     |
| itamtestsnft | 기린넷(테스트버전) |


```
// NFT 심볼 생성
ACTION create(name issuer, symbol_code symbol_name, string app_id);
- issuer: 심볼 발행 계정 (컨트랙트로 하는 것이 좋음)
- symbol_name: 심볼 이름
- app_id: 앱 아이디
```

```
// NFT 히스토리 기록용 액션
// 거래하지 않는 NFT까지 발행하기에는 리소스가 너무 낭비되므로 기록용 액션을 만듦.
ACTION nft(name owner, name owner_group, string game_user_id, string item_id, symbol_code symbol_name, string action, string reason)
- owner: NFT 소유 유저
- owner_group: 유저의 그룹 (eos or itam)
- game_user_id: 유저의 게임 아이디 (체크하지 않음)
- item_id: 아이템 아이디. uint64_t 형으로 JS Big Integer 표기 이슈로 문자열로 요청 받음
- symbol_name: 심볼 이름
- action: NFT 히스토리 행위 (issue,  burn ...)
- reason: NFT가 변경된 사유
```

```
// NFT 발행 (실제 테이블에 NFT 데이터를 추가함)
ACTION issue(name to, name to_group, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason)
- to: NFT를 발행할 계정
- to_group: NFT를 발행할 계정의 그룹 (eos or itam)
- nickname: NFT를 발행할 계정의 게임 닉네임
- symbol_name: 심볼 이름
- item_id: NFT 아이디
- item_name: NFT 이름
- group_id: NFT 그룹 아이디 (해당 NFT가 어떤 아이템인지를 가리키는 유니크 아이디 -> digital-asset-group 컬렉션 참고)
- options: NFT의 옵션
- duration: NFT가 유효한 초단위 기간 (0이면 영구제한 아이템. 기간이 지나면 삭제되는 것까지는 개발되지 않았음)
- transferable: 거래(전송) 가능 여부
- reason: 발행 사유
```

```
// NFT 발행 (실제 테이블에 NFT 데이터를 추가함)
// issue와 동일하게 동작하지만 다크타운 관련 부분들을 건드리지 않게 하기 위해 game_user_id 데이터를 포함하여 액션을 따로 추가함.
ACTION activate(name to, name to_group, string game_user_id, string nickname, symbol_code symbol_name, string item_id, string item_name, string group_id, string options, uint64_t duration, bool transferable, string reason);
- game_user_id: 유저의 게임 아이디 (체크하지 않음)
- 이외의 파라미터는 issue와 동일함.
```

```
// NFT 정보 수정
ACTION modify(name owner, name owner_group, symbol_code symbol_name, string item_id, string item_name, string options, uint64_t duration, bool transferable, string reason)
- owner: NFT 소유 유저
- owner_group: NFT 소유 유저의 그룹 (eos or itam)
- symbol_name: 심볼 이름
- item_id: NFT 아이디
- item_name: 수정할 NFT 이름
- options: 수정할 NFT 옵션
- durtaion: 수정할 기간
- transferable: 수정할 전송 가능 옵션
- reason: 수정 사유
```

```
// NFT 소유자 변경
ACTION changeowner(symbol_code symbol_name, string item_id, name owner, name owner_group, string nickname)
- symbol_name: 심볼 이름
- item_id: NFT 아이디
- owner: 변경할 소유자
- owner_group: 변경할 소유자 그룹 (eos or itam)
- nickname: 변경할 닉네임
```

```
// NFT 삭제
// 두 액션이 동일한 기능을 함.
ACTION burn(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason)
ACTION deactivate(name owner, name owner_group, symbol_code symbol_name, string item_id, string reason)
- owner: NFT 소유자 아이디
- owner_group: NFT 소유자의 그룹 (eos or itam)
- symbol_name: 심볼 이름
- item_id: NFT 아이디
- reason: 삭제 사유
```

```
// NFT 전체 삭제
ACTION burnall(symbol_code symbol)
- symbol: 심볼 이름
```     

```
// NFT 전송
// 화이트리스트에 추가된 것들만 전송 가능하다.
ACTION transfernft(name from, name to, symbol_code symbol_name, string item_id, string memo)
- from: 보낼 계정
- to: 받을 계정 (실제 EOS 계정)
- symbol_name: 심볼 이름
- memo: 메모

메모를 "|" 구분자를 이용하여 파싱하여 사용한다.
메모 구조
- nickname: NFT를 받을 계정의 닉네임
- to: NFT를 받을 받을 유저 아이디
- transfer_state: 전송 상태
```

```
// 화이트리스트에 NFT를 전송 가능한 EOS 계정 추가
ACTION addwhitelist(name account)
- account: EOS 계정
```

```
// 화이트리스트에 EOS 계정 제거
ACTION delwhitelist(name account)
- account: EOS 계정
```

```
// NFT 그룹 아이디 변경
ACTION changegroup(symbol_code symbol_name, string item_id, string group_id)
- symbol_name: 심볼 이름
- item_id: NFT 아이디
- group_id: NFT 그룹 아이디
```

```
// NFT 전송 시 내부적으로 호출되는 인라인 액션
ACTION receipt(name owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string options, uint64_t duration, bool transferable, asset payment_quantity, string state)
- owner: NFT 소유자
- owner_group: NFT 소유자 그룹 (eos or itam)
- app_id: 앱 아이디
- item_id: NFT 아이디
- nickname: NFT 소유자 닉네임
- group_id: NFT 그룹 아이디
- item_name: NFT 이름
- options: NFT 옵션
- duration: NFT 기간
- transferable: NFT 전송 가능 여부
- payment_quantity: 거래 금액
- state: 거래 상태
```