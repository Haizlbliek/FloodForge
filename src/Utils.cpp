#include "Utils.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <filesystem>

#define M_PI   3.141592653589
#define M_PI_2 1.570796326795

void fillRect(float x0, float y0, float x1, float y1) {
	glBegin(GL_QUADS);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x1, y1);
	glVertex2f(x0, y1);
	glEnd();
}

void strokeRect(float x0, float y0, float x1, float y1) {
	glBegin(GL_LINE_LOOP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x1, y1);
	glVertex2f(x0, y1);
	glEnd();
}

void drawLine(float x0, float y0, float x1, float y1, double thickness) {
	thickness /= 64.0;

	double angle = atan2(y1 - y0, x1 - x0);

	float a0x = x0 + cos(angle - M_PI_2) * thickness;
	float a0y = y0 + sin(angle - M_PI_2) * thickness;
	float b0x = x0 + cos(angle + M_PI_2) * thickness;
	float b0y = y0 + sin(angle + M_PI_2) * thickness;
	float a1x = x1 + cos(angle - M_PI_2) * thickness;
	float a1y = y1 + sin(angle - M_PI_2) * thickness;
	float b1x = x1 + cos(angle + M_PI_2) * thickness;
	float b1y = y1 + sin(angle + M_PI_2) * thickness;

	float c0x = x0 + cos(angle + M_PI) * thickness;
	float c0y = y0 + sin(angle + M_PI) * thickness;
	float c1x = x1 + cos(angle) * thickness;
	float c1y = y1 + sin(angle) * thickness;

	glBegin(GL_TRIANGLES);

	glVertex2f(a0x, a0y);
	glVertex2f(a1x, a1y);
	glVertex2f(b0x, b0y);

	glVertex2f(a1x, a1y);
	glVertex2f(b1x, b1y);
	glVertex2f(b0x, b0y);

	glVertex2f(a0x, a0y);
	glVertex2f(b0x, b0y);
	glVertex2f(c0x, c0y);

	glVertex2f(a1x, a1y);
	glVertex2f(b1x, b1y);
	glVertex2f(c1x, c1y);

	glEnd();
}

GLuint loadTexture(std::string filepath) {
	return loadTexture(filepath.c_str(), GL_NEAREST);
}

GLuint loadTexture(std::string filepath, int filter) {
	return loadTexture(filepath.c_str(), filter);
}

GLuint loadTexture(const char *filepath) {
	return loadTexture(filepath, GL_NEAREST);
}

GLuint loadTexture(const char *filepath, int filter) {
	int width, height, nrChannels;

	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (!data) {
		std::cerr << "Failed to load texture: " << filepath << std::endl;
		return 0;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	// glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);

	return textureID;
}

GLFWimage loadIcon(const char* filepath) {
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (!data) {
		std::cerr << "Failed to load texture: " << filepath << std::endl;
		return GLFWimage();
	}

	GLFWimage icon;

	icon.width = width;
	icon.height = height;
	icon.pixels = data;

	// Generate the texture
	// GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
	// glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	// glGenerateMipmap(GL_TEXTURE_2D);

	// Free the image data after uploading it to the GPU
	// stbi_image_free(data);

	return icon;
}

void saveImage(GLFWwindow *window, const char *fileName) {
	// Get the window size
	int width, height;
	glfwGetFramebufferSize(window, &width, &height); // Get the current framebuffer size

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	// Allocate memory for the pixel data
	unsigned char* pixels = new unsigned char[3 * width * height]; // 3 channels for RGB
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels); // Read pixel data from the framebuffer

	// Flip the image because OpenGL reads it upside down
	unsigned char* flippedPixels = new unsigned char[3 * width * height];
	for (int i = 0; i < height; ++i) {
		memcpy(flippedPixels + i * 3 * width, pixels + (height - i - 1) * 3 * width, 3 * width);
	}

	// Write the flipped image data to a PNG file
	stbi_write_png(fileName, width, height, 3, flippedPixels, width * 3);

	// Clean up allocated memory
	delete[] pixels;
	delete[] flippedPixels;
}

bool startsWith(const std::string &str, const std::string &prefix) {
	if (prefix.size() > str.size()) {
		return false;
	}
	return str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string &str, const std::string &suffix) {
	if (suffix.size() > str.size()) {
		return false;
	}
	return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string toLower(const std::string &str) {
	std::string output = str;
	std::transform(output.begin(), output.end(), output.begin(), ::tolower);

	return output;
}

std::string toUpper(const std::string &str) {
	std::string output = str;
	std::transform(output.begin(), output.end(), output.begin(), ::toupper);

	return output;
}

std::string findFileCaseInsensitive(const std::string &directory, const std::string &fileName) {
	for (const auto &entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			const std::string entryFileName = entry.path().filename().string();
			if (toLower(entryFileName) == toLower(fileName)) {
				return entry.path().string();
			}
		}
	}
	return "";
}





#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

std::string ShowFileDialog(const std::string &filter, bool mustExist = true) {
	OPENFILENAME ofn;
	char szFile[260] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter.c_str();
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | (mustExist ? OFN_FILEMUSTEXIST : 0);

	if (GetOpenFileName(&ofn)) {
		return std::string(ofn.lpstrFile);
	}

	DWORD error = CommDlgExtendedError();
	if (error != 0) {
		std::cerr << "Error: " << error << std::endl;
	}

	return "";
}

std::string OpenNewFileDialog(const std::string &types) {
	return ShowFileDialog(types, false);
}

std::string OpenNewFileDialog() {
	return ShowFileDialog("Floodwaters Level (*.level)\0*.LEVEL\0", false);
}

std::string OpenFileDialog(const std::string &types) {
	std::string filter = types + '\0';
	for (char &c : filter) {
		if (c == '|') c = '\0';
	}
	return ShowFileDialog(filter);
}

std::string OpenFileDialog() {
	return ShowFileDialog("Floodwaters Level (*.level)\0*.LEVEL\0All Files (*.*)\0*.*\0");
}

std::string OpenDirectoryDialog() {
	return OpenDirectoryDialog("Select Folder");
}

std::string OpenDirectoryDialog(const std::string &title) {
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpszTitle = title.c_str();
	
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL) {
		char path[MAX_PATH];
		if (SHGetPathFromIDList(pidl, path)) {
			CoTaskMemFree(pidl);  // Free the PIDL allocated by SHBrowseForFolder
			return std::string(path);
		}
		CoTaskMemFree(pidl);  // Free the PIDL even if the path was not obtained
	}

	return "";  // Return an empty string if no directory was selected
}

bool verifyBox(std::string text) {
	return MessageBox(NULL, text.c_str(), "", MB_YESNO) == IDYES;
}

#elif __linux__
#warning UNVERIFIED LINUX SUPPORT, PLEASE REPORT ERRORS

#include <gtk/gtk.h>
#include <iostream>
#include <string>

std::string OpenNewFileDialog(std::string types) {
    GtkWidget *dialog;
    GtkFileFilter *filter;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gchar *filename;

    gtk_init(NULL, NULL);

    dialog = gtk_file_chooser_dialog_new("Open File", NULL, action,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Open", GTK_RESPONSE_ACCEPT,
                                        NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, types.c_str());
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    // Run dialog
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        std::string filePath = filename;
        g_free(filename);
        gtk_widget_destroy(dialog);
        return filePath;
    }

    gtk_widget_destroy(dialog);
    return "";
}

std::string OpenNewFileDialog() {
    return OpenNewFileDialog("*.level");
}

std::string OpenFileDialog(std::string types) {
    return OpenNewFileDialog(types);
}

std::string OpenFileDialog() {
    return OpenNewFileDialog("*.level");
}

std::string OpenDirectoryDialog() {
	OpenDirectoryDialog("Select Folder");
}

std::string OpenDirectoryDialog(const std::string &title) {
	std::cerr << "OpenDirectoryDialog not implemented on Linux yet (sorry!)\n";

	return "";
}

bool verifyBox(std::string text) {
    GtkWidget *dialog;
    gint response;

    gtk_init(NULL, NULL);

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO, text.c_str());

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return response == GTK_RESPONSE_YES;
}
#else

#error Will not work for your OS, sorry!

#endif



// Gl Functions

void glColour(Colour colour) {
	glColor4f(colour.R(), colour.G(), colour.B(), colour.A());
}

void glColor(Colour color) {
	glColour(color);
}