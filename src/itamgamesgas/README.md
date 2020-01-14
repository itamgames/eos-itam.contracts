## 정산 계정

|      계정     |      환경      |
| ------------ | ------------- |
| itamgamesgas |   메인넷/기린넷   |


```
// 앱의 정산 받을 EOS 계정 설정
ACTION setsettle(string app_id, name settle_account)
- app_id: 앱 아이디
- settle_account: 정산금액을 받을 EOS 계정
```

```
// 정산
ACTION claimsettle(string app_id)
- app_id: 정산할 앱 아이디
```

```
// 정산 영수증
ACTION receiptstle(string app_id, name settle_account, asset quantity)
- app_id: 앱 아이디
- settle_account: 정산 계정
- quantity: 금액
```

```
// eosio.token의 transfer 이벤트를 트리거 하여 정산 받을 금액을 업데이트 함.
ACTION transfer(uint64_t from, uint64_t to)
```
