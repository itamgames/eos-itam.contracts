# DEX
## NFT 거래 컨트랙트

|      계정     |      환경      |
| ------------ | ------------- |
| itamstoredex |      메인넷     |
| itamtestsdex | 메인넷(테스트버전) |
| itamgamesdex |      기린넷     |
| itamtestsdex | 기린넷(테스트버전) |


```
// NFT 판매 등록
// DEX 컨트랙트에 NFT를 보낸다.
ACTION sellorder(name owner, symbol_code symbol_name, string item_id, asset quantity)
- owner: NFT 소유자
- symbol_name: 심볼 이름
- item_id: NFT 아이디
- quantity: 판매 금액
```

```
// NFT 판매 등록 취소
ACTION cancelorder(name owner, symbol_code symbol_name, string item_id)
- owner: NFT 소유자
- symbol_name: 심볼 이름
- item_id: NFT 아이디
```

```
// 거래 중인 리스트 제거
// 데이터 싱크가 안 맞았던 경우 때문에 추가한 액션.
ACTION deleteorders(symbol_code symbol_name, vector<string> item_ids)
- symbol_name: 심볼 이름
- item_ids: 지울 거래 정보 리스트
```

```
// 거래 중인 리스트 전체 제거
ACTION resetorders(symbol_code symbol_name)
- symbol_name: 심볼 이름
```

```
// 거래소에서 이용할 토큰 등록
ACTION settoken(name contract_name, string symbol_name, uint32_t precision)
- contract_name: 토큰 계정
- symbol_name: 심볼 이름
- precision: 소수점 자리 개수
```

```
// 토큰 제거
ACTION deletetoken(string symbol_name, uint32_t precision)
- symbol_name: 심볼 이름
- precision: 소수점 자리 개수
```

```
// 수수료, 정산 비율 설정
ACTION setconfig(string symbol_name, uint64_t fees_rate, uint64_t settle_rate)
- symbol_name: 심볼 이름
- fees_rate: 거래 수수료 비율
- settle_rate: 게임사가 받을 정산 수수료 비율
```

```
// 거래 영수증
// 거래 시 호출되는 인라인 액션
ACTION receipt(name owner, name owner_group, uint64_t app_id, uint64_t item_id, string nickname, uint64_t group_id, string item_name, string options, uint64_t duration, bool transferable, asset payment_quantity, asset settle_quantity_to_vendor, asset settle_quantity_to_itam, string state)
- owner: 새로운 NFT 소유자
- owner_group: NFT 소유자 그룹 (eos or itam)
- app_id: 앱 아이디
- item_id: NFT 아이디
- nickname: 닉네임
- group_id: NFT 그룹 아이디
- item_name: NFT 이름
- options: NFT 옵션
- duration: 기간
- transferable: 전송 가능 여부
- payment_quantity: 금액
- settle_quantity_to_vendor: 게임사가 받을 정산 금액
- settle_quantity_to_itam: ITAM이 받을 정산 금액
- state: 거래 상태
```