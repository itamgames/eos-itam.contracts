## 유료앱/인앱 컨트랙트

|      계정     |      환경      |
| ------------ | ------------- |
| itamstoreapp |      메인넷     |
| itamtestsapp | 메인넷(테스트버전) |
| itamgamesapp |      기린넷     |
| itamtestsapp | 기린넷(테스트버전) |


```
// 유료앱 등록
ACTION registapp(string appId, name owner, asset price, string params)
- appId: 유료앱 아이디
- owner: 앱 소유자(사용 x)
- price: 가격
- params: 인앱 베포 데이터 (JSON Stringify, 사용되지 말아야 함)
```

```
// 유료앱 삭제
ACTION deleteapp(string appId)
- appId: 유료앱 아이디
```

```
// 인앱 등록
ACTION registitems(string params)
- params: 인앱 파라미터 리스트 (JSON Stringify)
params 구조 
{
    appId: string
    items: [
        {
            itemId: string
            itemName: string
            price: string
        }
    ]
}
```

```
// 인앱 삭제
ACTION deleteitems(string params)
- params: 인앱 파라미터 리스트 (JSON Stringify)
params 구조 
{
    appId: string
    items: [
        {
            itemId: string
        }
    ]
}
```

```
// 아이템 수정
ACTION modifyitem(string appId, string itemId, string itemName, asset price)
- appId: 앱 아이디
- itemId: 아이템 아이디
- itemName: 수정할 아이템 이름
- price: 수정할 아이템 가격
```

```
// 유료앱/인앱 구매 영수증
ACTION receiptapp(uint64_t appId, name from, name owner, name ownerGroup, asset quantity)
ACTION receiptitem(uint64_t appId, uint64_t itemId, string itemName, name from, name owner, name ownerGroup, asset quantity)
- appId: 앱 아이디
- itemId: 아이템 아이디
- itemName: 아이템 이름
- from: 구매 EOS 계정
- owner: 구매 유저 아이디
- ownerGroup: 구매 유저 그룹 (eos or itam)
- quantity: 구매 금액
```

```
// 아이템 사용 기록
ACTION useitem(string appId, string itemId, string memo)
- appId: 앱 아이디
- itemId: 아이템 아이디
- memo: 메모
```

```
// 결제 수수료, 환불 가능일 설정
ACTION setconfig(uint64_t ratio, uint64_t refundableDay)
- ratio: 게임사가 가져갈 수수료 퍼센트
- refundableDay: 환불 가능일
```

```
// 유료앱 환불
ACTION refundapp(string appId, name owner, uint64_t paymentTimestamp)
- appId: 앱 아이디
- owner: 결제한 유저
- paymentTimestamp: 결제 블록 타임스탬프
```

```
// 인앱 환불
ACTION refunditem(string appId, string itemId, name owner, uint64_t paymentTimestamp)
- appId: 앱 아이디
- itemId: 인앱 아이디
- owner: 결제한 유저
- paymentTimestamp: 결제 블록 타임스탬프
```

```
// 환불 불가능하도록 상태 변경 (결제 시 delay action 으로 호출됨)
ACTION defconfirm(uint64_t appId, name owner)
- appId: 앱 아이디
- owner: 결제한 유저
```

```
// 환불 불가능하도록 상태 변경 (환불기간 지난 전체 유저의 데이터를 변경)
ACTION confirmall(string appId)
- appId: 앱 아이디
```