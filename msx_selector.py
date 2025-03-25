import pygame
import os
import sys
import random  # 추가

class MSXSelector:
    def __init__(self):
        # Set SDL video driver before pygame init
        # os.environ['SDL_VIDEODRIVER'] = 'kmsdrm'
        # os.environ["SDL_VIDEODRIVER"] = "dummy"
        try:
            pygame.init()
        except pygame.error:
            os.environ['SDL_VIDEODRIVER'] = 'fbdev'
            try:
                pygame.init()
            except pygame.error:
                os.environ['SDL_VIDEODRIVER'] = 'directfb'
                pygame.init()

        # Get the current display info
        display_info = pygame.display.Info()
        self.screen_width = display_info.current_w
        self.screen_height = display_info.current_h
        
        try:
            self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
        except pygame.error:
            # Fallback to a different resolution if full resolution fails
            self.screen_width = 1280
            self.screen_height = 720
            self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))

        pygame.display.set_caption("MSX Machine Selector")

        # Get system font
        available_fonts = pygame.font.get_fonts()
        preferred_fonts = ["khmerossystem", 'arial', 'helvetica', 'segoeui', 'malgun']
        self.font_name = next((f for f in preferred_fonts if f in available_fonts), available_fonts[0])
        # Adjust font sizes for fullscreen
        base_font_size = int(self.screen_height * 0.0625)  # 6.25% of screen height
        self.font = pygame.font.SysFont(self.font_name, base_font_size)
        self.button_font = pygame.font.SysFont(self.font_name, int(base_font_size * 0.5))

        # Load images and prepare categories
        self.images_dir = "images"
        self.images = []
        self.image_names = []
        self.categories = set()
        self.current_category = None
        self.filtered_indices = []
        self.load_images_and_categories()

        # Button settings
        self.buttons = []
        self.create_category_buttons()

        # Display settings
        self.current_index = 0
        self.target_x = 0
        self.current_x = 0
        self.scroll_speed = 15
        self.image_width = 400
        self.image_spacing = 50

    def load_images_and_categories(self):
        with open('machines', 'r') as f:
            machines = f.readlines()
            
        # Extract categories
        for machine in machines:
            if ' - ' in machine:
                category = machine.split(' - ')[0].strip()
                if category == "MSX":
                    category = "MSX1"
                self.categories.add(category)

        # Load images
        for filename in os.listdir(self.images_dir):
            if filename.endswith(('.jpg', '.png')):
                try:
                    image_path = os.path.join(self.images_dir, filename)
                    image = pygame.image.load(image_path)
                    image = pygame.transform.scale(image, (400, 300))
                    self.images.append(image)
                    self.image_names.append(filename.replace('_', ' ').rsplit('.', 1)[0])
                except:
                    import traceback
                    traceback.print_exc()
                    print(f"Failed to load {filename}")
        
        self.filtered_indices = list(range(len(self.images)))

    def filter_by_category(self, category):
        if category == self.current_category:
            self.current_category = None
            self.filtered_indices = list(range(len(self.images)))
        else:
            self.current_category = category
            self.filtered_indices = [i for i, name in enumerate(self.image_names)
                                   if category in name.split(' - ')[0] or
                                   (category == "MSX1" and name.startswith("MSX - "))]
        self.current_index = 0

    def create_category_buttons(self):
        button_height = 40
        min_padding = 8  # 버튼 사이 간격 약간 증가
        text_padding = 16  # 텍스트 좌우 여백
        
        # 버튼 크기 계산
        buttons_info = []
        total_width = 0
        for category in sorted(self.categories):
            text_width = self.button_font.size(category)[0]
            button_width = text_width + text_padding
            buttons_info.append((category, button_width))
            total_width += button_width
        
        # 전체 너비 계산 (버튼들 + 패딩)
        total_width += min_padding * (len(buttons_info) - 1)
        
        # 시작 위치를 화면 중앙 기준으로 계산
        x = (self.screen_width - total_width) // 2
        y = 10
        
        for category, width in buttons_info:
            self.buttons.append({
                'rect': pygame.Rect(x, y, width, button_height),
                'text': category,
                'key': str(len(self.buttons) + 1),
                'selected': False,
                'radius': 8
            })
            x += width + min_padding

        # 배경 패턴 설정 추가
        self.pattern_size = 32
        self.pattern_colors = [
            (60, 60, 80),    # 연한 남색
            (80, 60, 60),    # 연한 적색
            (60, 80, 60),    # 연한 녹색
            (70, 70, 70)     # 연한 회색
        ]
        self.generate_background_pattern()

    def generate_background_pattern(self):
        pattern_types = ['dots', 'grid', 'diagonal', 'circles']
        self.current_pattern = random.choice(pattern_types)
        self.pattern_offset = random.randint(0, self.pattern_size)
        self.pattern_color = random.choice(self.pattern_colors)
        self.pattern_alpha = random.randint(25, 40)  # 투명도 증가

    def draw_background(self):
        if self.current_pattern == 'dots':
            for x in range(-self.pattern_offset, self.screen_width, self.pattern_size):
                for y in range(-self.pattern_offset, self.screen_height, self.pattern_size):
                    pygame.draw.circle(self.screen, self.pattern_color, (x, y), 2)
        
        elif self.current_pattern == 'grid':
            for x in range(-self.pattern_offset, self.screen_width, self.pattern_size):
                pygame.draw.line(self.screen, self.pattern_color, (x, 0), (x, self.screen_height))
            for y in range(-self.pattern_offset, self.screen_height, self.pattern_size):
                pygame.draw.line(self.screen, self.pattern_color, (0, y), (self.screen_width, y))
        
        elif self.current_pattern == 'diagonal':
            for i in range(-self.pattern_offset, self.screen_width + self.screen_height, self.pattern_size):
                pygame.draw.line(self.screen, self.pattern_color, (i, 0), (i - self.screen_height, self.screen_height))
        
        elif self.current_pattern == 'circles':
            for x in range(-self.pattern_offset, self.screen_width, self.pattern_size * 2):
                for y in range(-self.pattern_offset, self.screen_height, self.pattern_size * 2):
                    pygame.draw.circle(self.screen, self.pattern_color, (x, y), self.pattern_size // 2, 1)

    def draw(self):
        self.screen.fill((15, 15, 20))  # 배경색을 약간 밝게 조정
        self.draw_background()
        
        # Draw category buttons with shortcuts
        shortcut_font = pygame.font.SysFont(self.font_name, 16)
        for button in self.buttons:
            color = (255, 200, 0) if button['text'] == self.current_category else (128, 128, 128)
            
            # Draw rounded rectangle
            rect = button['rect']
            radius = button['radius']
            pygame.draw.rect(self.screen, color, rect) #, border_radius=radius)
            
            # Draw main text
            text = self.button_font.render(button['text'], True, (0, 0, 0))
            text_rect = text.get_rect(center=(rect.centerx, rect.centery - 5))
            self.screen.blit(text, text_rect)
            
            # Draw shortcut key
            shortcut = shortcut_font.render(f"[{button['key']}]", True, (0, 0, 0))
            shortcut_rect = shortcut.get_rect(center=(rect.centerx, rect.bottom - 8))
            self.screen.blit(shortcut, shortcut_rect)

        # Draw images
        center_y = self.screen_height // 2
        visible_range = 5

        if self.filtered_indices:  # Only draw if we have filtered items
            # First pass: Draw all images except the selected one
            for i in range(visible_range, -visible_range - 1, -1):
                idx = self.current_index + i
                if 0 <= idx < len(self.filtered_indices):
                    image_index = self.filtered_indices[idx]
                    if idx != self.current_index:
                        distance = abs(i)
                        scale = max(0.5, 1 - (distance * 0.2))
                        scaled_width = int(self.image_width * scale)
                        scaled_height = int(300 * scale)
                        
                        x = self.screen_width//2 - scaled_width//2 + i * (self.image_width//2) + self.current_x
                        y = center_y - 150 + (distance * distance * 10)
                        
                        scaled_image = pygame.transform.scale(self.images[image_index], (scaled_width, scaled_height))
                        alpha = max(128, 255 - (distance * 40))
                        scaled_image.set_alpha(alpha)
                        
                        self.screen.blit(scaled_image, (x, y))
                        
                        if distance <= 2:
                            font_size = int(48 * scale)
                            distance_font = pygame.font.SysFont(self.font_name, font_size)
                            text = distance_font.render(self.image_names[image_index], True, (255, 255, 255))
                            text.set_alpha(alpha)
                            text_rect = text.get_rect(center=(x + scaled_width//2, y + scaled_height + 50))
                            self.screen.blit(text, text_rect)

            # Second pass: Draw the selected image
            if 0 <= self.current_index < len(self.filtered_indices):
                image_index = self.filtered_indices[self.current_index]
                scaled_image = pygame.transform.scale(self.images[image_index], (self.image_width, 300))
                x = self.screen_width//2 - self.image_width//2 + self.current_x
                y = center_y - 150
                
                pygame.draw.rect(self.screen, (255, 0, 0), 
                               (x - 5, y - 5, self.image_width + 10, 310), 3)
                
                self.screen.blit(scaled_image, (x, y))
                
                text = self.font.render(self.image_names[image_index], True, (255, 255, 255))
                text_rect = text.get_rect(center=(x + self.image_width//2, y + 350))
                self.screen.blit(text, text_rect)

        pygame.display.flip()

    def run(self):
        clock = pygame.time.Clock()
        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                    running = False
                elif event.type == pygame.MOUSEBUTTONDOWN:
                    if event.button == 1:
                        # Check if any category button was clicked
                        mouse_pos = pygame.mouse.get_pos()
                        for button in self.buttons:
                            if button['rect'].collidepoint(mouse_pos):
                                self.filter_by_category(button['text'])
                                break
                        else:
                            # Handle machine selection and quit pygame before launching blueMSX
                            selected_machine = self.image_names[self.filtered_indices[self.current_index]]
                            pygame.quit()
                            print(f"Selected: {selected_machine}")
                            # os.system(f'./bluemsx-pi /machine "{selected_machine}" /romtype1 msxbus /romtype2 msxbus')
                            return selected_machine
                elif event.type == pygame.KEYDOWN:
                    if event.unicode.isdigit() and event.unicode != '0':
                        key_index = int(event.unicode) - 1
                        if key_index < len(self.buttons):
                            self.filter_by_category(self.buttons[key_index]['text'])
                    elif event.key == pygame.K_LEFT and self.current_index > 0:
                        self.current_index -= 1
                    elif event.key == pygame.K_RIGHT and self.current_index < len(self.filtered_indices) - 1:
                        self.current_index += 1
                    elif event.key == pygame.K_RETURN:
                        selected_machine = self.image_names[self.filtered_indices[self.current_index]]
                        pygame.quit()
                        print(f"Selected: {selected_machine}")
                        # os.system(f'./bluemsx-pi /machine "{selected_machine}" /romtype1 msxbus /romtype2 msxbus')
                        return selected_machine

            # Mouse wheel scrolling
            mouse_rel = pygame.mouse.get_rel()
            if pygame.mouse.get_pressed()[0]:  # Left mouse button
                self.current_x += mouse_rel[0]
                if abs(mouse_rel[0]) > self.image_width//2:
                    if mouse_rel[0] > 0 and self.current_index > 0:
                        self.current_index -= 1
                    elif mouse_rel[0] < 0 and self.current_index < len(self.images) - 1:
                        self.current_index += 1

            self.draw()
            clock.tick(60)

        pygame.quit()
        return None

import os, sys

if __name__ == "__main__":
    while True:
        selector = MSXSelector()
        selected_machine = selector.run()
        if selected_machine:
           print(f"Final selection: {selected_machine}")
           os.system(f'./bluemsx-pi /machine "{selected_machine}" /romtype1 msxbus /romtype2 msxbus')
        else:
            sys.exit();
