#!/usr/bin/env python3
import os
import sys
import subprocess
import platform
from prompt_toolkit import Application
from prompt_toolkit.layout.containers import HSplit, Window
from prompt_toolkit.layout.controls import FormattedTextControl
from prompt_toolkit.layout.layout import Layout
from prompt_toolkit.key_binding import KeyBindings
from prompt_toolkit.widgets import TextArea, Frame
from prompt_toolkit.filters import Condition
from prompt_toolkit.styles import Style

# 현재 디렉토리 설정
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# 설정 및 상수
ITEM_FILE = "./item"
MSX_MACHINES_FILE = "./msxmachines"
RASPBERRY_MODEL_FILE = "/proc/device-tree/model"
FILTER_FILE = "./filter"  # 필터 문자열 저장 파일 추가

# 터미널 전체 화면 설정/해제 함수
def set_fullscreen(enable=True):
    if sys.platform != 'win32':  # Linux/Unix 시스템에서만 실행
        try:
            if enable:
                subprocess.run(['wmctrl', '-r', ':ACTIVE:', '-b', 'add,fullscreen'], 
                              stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
            else:
                subprocess.run(['wmctrl', '-r', ':ACTIVE:', '-b', 'remove,fullscreen'], 
                              stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        except Exception as e:
            print(f"Fullscreen toggle error: {e}")

# 머신 목록 로드
def load_machines():
    machines = []
    try:
        with open(MSX_MACHINES_FILE, 'r') as f:
            machines = [line.strip() for line in f if line.strip()]
    except FileNotFoundError:
        print(f"Error: {MSX_MACHINES_FILE} not found")
        sys.exit(1)
    return machines

# 마지막 선택 항목 로드
def load_last_item():
    try:
        with open(ITEM_FILE, 'r') as f:
            return f.read().strip()
    except:
        return "1"

# 마지막 선택 항목 저장
def save_item(item):
    with open(ITEM_FILE, 'w') as f:
        f.write(str(item))

# Raspberry Pi 모델 확인
def get_raspberry_model():
    if os.path.exists(RASPBERRY_MODEL_FILE):
        try:
            with open(RASPBERRY_MODEL_FILE, 'r') as f:
                model = f.read().split()[2]
                return model
        except:
            pass
    return None

# 필터 문자열 로드 함수 추가
def load_filter():
    try:
        with open(FILTER_FILE, 'r') as f:
            return f.read().strip()
    except:
        return ""

# 필터 문자열 저장 함수 추가
def save_filter(filter_text):
    with open(FILTER_FILE, 'w') as f:
        f.write(filter_text)

# MSX 에뮬레이터 실행
def run_msx_emulator(machine):
    raspberry_model = get_raspberry_model()
    
    # MSXBUS 설정
    if raspberry_model == "4":
        bmsxbus = "zmxbus"
        omsxbus = "ZemmixBus"
    else:
        bmsxbus = "zmxdrive"
        omsxbus = "ZemmixDrive"
        # SDCARD 환경 변수 설정
        try:
            result = subprocess.run(
                "lsblk -l | grep vfat | awk '{print $7}' | sed -n 1p",
                shell=True, capture_output=True, text=True
            )
            sdcard = result.stdout.strip()
            if sdcard:
                os.environ["SDCARD"] = sdcard
        except:
            pass
    # 에뮬레이터 실행
    try:
        BLUEMSX = "./bluemsx-pi"
        OPENMSX = "./openmsx"
        if os.path.exists(BLUEMSX):
            cmd = [BLUEMSX, "/machine", machine, "/romtype1", bmsxbus, "/romtype2", bmsxbus]
        elif os.path.exists(OPENMSX):
            cmd = [OPENMSX, "-machine", machine, "-ext", omsxbus]
        # print(' '.join(cmd))
        subprocess.run(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
    except Exception as e:
        import traceback
        print(f"Error running emulator: {traceback.print_exc()}")

# 메인 애플리케이션 클래스
class MSXMenuApp:
    def __init__(self):
        self.machines = load_machines()
        self.filtered_machines = self.machines.copy()
        self.selected_index = 0
        self.filter_text = load_filter()  # 저장된 필터 로드
        self.last_item = load_last_item()
        
        # 키 바인딩 설정
        self.kb = KeyBindings()
        
        @self.kb.add('c-c')  # ESC 키 추가
        @self.kb.add('escape')
        def _(event):
            event.app.exit()
        
        @self.kb.add('enter')
        def _(event):
            if 0 <= self.selected_index < len(self.filtered_machines):
                selected_machine = self.filtered_machines[self.selected_index]
                # 선택한 항목 번호 저장
                machine_index = self.machines.index(selected_machine) + 1
                save_item(machine_index)
                # 현재 필터 문자열 저장
                save_filter(self.filter_text)
                # event.app.exit()
                run_msx_emulator(selected_machine)
                # 프로그램 재시작
                # os.execv(sys.executable, ['python'] + sys.argv)
        
        @self.kb.add('up')
        def _(event):
            self.selected_index = max(0, self.selected_index - 1)
            # UI 업데이트를 위해 레이아웃 무효화
            event.app.invalidate()
        
        @self.kb.add('down')
        def _(event):
            # 수정: 최대값 계산 오류 수정
            if self.filtered_machines:
                self.selected_index = min(self.selected_index + 1, len(self.filtered_machines) - 1)
            # UI 업데이트를 위해 레이아웃 무효화
            event.app.invalidate()
        
        # pageup, pagedown 키 추가
        @self.kb.add('pageup')
        @self.kb.add('left')
        def _(event):
            # 페이지 높이만큼 위로 이동 (visible_items 값 사용)
            visible_items = 20
            self.selected_index = max(0, self.selected_index - visible_items)
            event.app.invalidate()
        
        @self.kb.add('pagedown')
        @self.kb.add('right')
        def _(event):
            # 페이지 높이만큼 아래로 이동 (visible_items 값 사용)
            visible_items = 20
            if self.filtered_machines:
                self.selected_index = min(self.selected_index + visible_items, len(self.filtered_machines) - 1)
            event.app.invalidate()
        
        # 필터 텍스트 영역 부분 업데이트
        # 필터 텍스트 영역
        self.filter_area = TextArea(
            height=1,
            prompt="Filter: ",
            style="class:filter",
            multiline=False,
            wrap_lines=False,
            text=self.filter_text,  # 저장된 필터 텍스트 설정
        )
        
        # 커서 위치를 텍스트 끝으로 설정
        self.filter_area.buffer.cursor_position = len(self.filter_text)

        # prompt-toolkit 3.0.50에서는 on_text_changed 대신 buffer.on_text_changed 이벤트 사용
        self.filter_area.buffer.on_text_changed += self.filter_changed
        self.filter_changed(self.filter_area.buffer)
        # 머신 목록 표시 영역
        self.machine_list = FormattedTextControl(self.get_machine_list)
        
        # 레이아웃 설정
        raspberry_model = get_raspberry_model()
        if raspberry_model:
            title = f"ZemmixDrive - Raspberry Pi {raspberry_model} MSX"
        else:
            title = "ZemmixDrive - BlueberryMSX"
        
        # 필터를 위로 이동하고 레이아웃 조정
        root_container = HSplit([
            self.filter_area,  # 필터를 위로 이동
            Frame(
                title=title, 
                body=Window(
                    self.machine_list,
                    height=None,  # None으로 설정하여 사용 가능한 모든 공간 사용
                    width=None    # None으로 설정하여 사용 가능한 모든 공간 사용
                )
            )
        ])
        
        # 스타일 설정
        style = Style.from_dict({
            'filter': '#ansiblue',
            'selected': 'reverse',
        })
        
        # 애플리케이션 생성
        self.app = Application(
            layout=Layout(root_container),
            key_bindings=self.kb,
            style=style,
            full_screen=True,  # 전체 화면 모드 비활성화
            mouse_support=True
        )
        
        # 초기 필터 적용
        try:
            last_index = int(self.last_item) - 1
            if 0 <= last_index < len(self.machines):
                # 마지막으로 선택한 머신 찾기
                last_machine = self.machines[last_index]
                # 필터링된 목록에서 해당 머신의 인덱스 찾기
                if last_machine in self.filtered_machines:
                    self.selected_index = self.filtered_machines.index(last_machine)
                else:
                    self.selected_index = 0
        except:
            self.selected_index = 0
    
    def filter_changed(self, buffer):
        # prompt-toolkit 3.0.50에서는 이벤트 객체가 전달됨
        filtered_machines = [
            machine for machine in self.machines 
            if buffer.text.lower() in machine.lower()
        ]
        if len(filtered_machines):
            self.filter_text = buffer.text.lower()
            self.filtered_machines = filtered_machines
            self.selected_index = min(self.selected_index, len(self.filtered_machines) - 1)
        else:
            buffer.text = buffer.text[:-1]
    
    def get_machine_list(self):
        result = []
        
        if not self.filtered_machines:
            result.append(("", "\n No machines match the filter \n"))
            return result
        
        # 상단 여백 추가
        result.append(("", "\n"))
        
        # 화면에 표시할 항목 수 계산 (스크롤 구현)
        # 터미널 크기에 따라 동적으로 visible_items 조정
        try:
            # 터미널 크기 가져오기
            import shutil
            terminal_size = shutil.get_terminal_size()
            # 필터 영역(1줄), 프레임 테두리(2줄), 스크롤 표시기(최대 2줄), 여백(2줄) 고려
            visible_items = terminal_size.lines - 7
            # 최소값 보장
            visible_items = max(10, visible_items)
        except:
            # 기본값 사용
            visible_items = 20
            
        total_items = len(self.filtered_machines)
        
        # 현재 선택된 항목이 항상 화면에 보이도록 시작 인덱스 계산
        start_idx = max(0, self.selected_index - (visible_items // 2))
        end_idx = min(total_items, start_idx + visible_items)
        
        # 시작 인덱스 조정 (끝에 도달했을 때)
        if end_idx == total_items:
            start_idx = max(0, total_items - visible_items)
        
        # 스크롤 표시기 추가
        if start_idx > 0:
            result.append(("", "  ↑\n"))
        
        # 항목 표시
        for i in range(start_idx, end_idx):
            machine = self.filtered_machines[i]
            if i == self.selected_index:
                result.append(("class:selected", f"  {i+1}. {machine}  \n"))
            else:
                result.append(("", f"  {i+1}. {machine}  \n"))
        
        # 하단 스크롤 표시기 추가
        if end_idx < total_items:
            result.append(("", "  ↓\n"))
        
        # 하단 여백 추가
        result.append(("", "\n"))
        
        return result
    
    def run(self):
        try:
            # 시작 시 전체 화면으로 설정
            set_fullscreen(True)
            self.app.run()
        finally:
            # 종료 시 전체 화면 해제 및 터미널 설정 복원
            set_fullscreen(False)
            # 터미널 설정 복원
            if sys.platform == 'win32':
                # Windows에서는 mode con 명령어로 에코 복원
                subprocess.run('mode con', shell=True)
            else:
                # Linux/Unix에서는 stty 명령어로 에코 복원
                subprocess.run('stty sane', shell=True)

if __name__ == "__main__":
    app = MSXMenuApp()
    app.run()
