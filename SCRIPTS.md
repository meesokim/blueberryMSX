msxplay는 가장 쉽게 MSX 게임을 플레이할 수 있는 에뮬레이터임.
windows와 ubuntu에서 모두 실행가능하기 위해서 SDL2 기반으로 구현함
blueberryMSX의 코드 구현을 바탕을 msxplay의 VDP를 구현했는데 필요한 코드는 직접 복사해서 활용하도록 해줘
현재 두가지 문제가 있어.
1. VRAM으로 부터 RGB 데이터를 생성하는데 visual error가 있음. 
2. rom 파일의 특성에 따라 ROM mapper를 선택하는 방법이 명확하지 않음.
이 두가지 문제는 새로운 emulator를 만든다고 해도 항상 발생할 수 있는 문제야.
그리고, VRAM 데이터로 부터 RGB 데이터를 생성하는데 문제가 없다는 것을 검증하기 위해서 blueberryMSX의 VDP 구현 코드를 기반으로 verifier를 만들어줘.
verifier는 샘플 VRAM 데이터와 RGB 데이터를 게임으로 부터 만들어 DB를 만들어 둔다.
새로운 에뮬레이터를 만들었다고 하면 VRAM의 셈플데이터를 주입하여 실제 생성된 RGB 데이터를 주출하여 검증하면 어떤 문제가 있는지 확인이 됨
rom mapper verifier는 rom 파일 셈플을 기반으로 실제 rom mapper를 제대로 결정했는지를 평가한다.
rom 파일 (압축이 풀린)의 SHA1 코드와 올바른 rom mapper의 매칭 DB를 만들어둔다.
이를 통해서 rom mapper를 선정하면 잘못된 rom 적재로 인한 오류는 발생하지 않는다.

셈플 ROM은 MSX1, MSX2 디렉토리 안에 있다.
