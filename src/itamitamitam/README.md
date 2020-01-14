## 중앙계정

|      계정     |      환경      |
| ------------ | ------------- |
| itamitamitam |   메인넷/기린넷   |


```
// 입금
// eosio.token 시스템 컨트랙트의 transfer 이벤트를 감지하여 입금 처리함
ACTION transfer(uint64_t from, uint64_t to)
```

```
// 출금
ACTION transferto(name from, name to, asset quantity, string memo)
- from: 송금 유저
- to: 입금 EOS 계정
- quantity: 금액
- memo: 메모
```

```
// 유저 잔액 수정
ACTION modbalance(name owner, asset quantity)
- owner: 유저 아이디
- quantity: 바꿀 잔액
```