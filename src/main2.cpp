#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "funcs.h"

#include <vector>
#include <thread>
#include <cstdint>
#include <complex>
#include <algorithm>
#include <random>
#include <cmath>

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"

void run_gui()
{
    std::vector<uint8_t> bits(2000);
    std::vector<std::complex<float>> symbols(2000);
    std::vector<std::complex<float>> buffer(2000);
    std::vector<std::complex<float>> signal(2000);
    std::vector<std::complex<float>> demoded(2000);
    std::vector<std::complex<float>> equalized(2000);
    std::vector<uint8_t> received_bits(2000);
    std::vector<float> buffer_spectrum(2000);
    std::vector<float> signal_spectrum(2000);
    std::vector<float> buffer_freqs(2000);
    std::vector<float> signal_freqs(2000);

    int start = 0;
    int L = 1440;
    float coef = 0.0f;
    bool RT = false;
    bool update = true;
    bool always_random = false;
    float sample_rate = 1.92e6;
    bool random_paths = false;

    OFDMConfig base;
    MultipathChannel channel;
    float noise_db = -174 + 10 * std::log10(sample_rate) + 6;
    channel.set_noise(noise_db);
    channel.set_paths();
    int N = base.n_subcarriers;
    int PS = base.pilots_spacing;

    generate_bits(bits, 1440);
    qpsk_mapper_3gpp(bits, symbols);
    ofdm(symbols, buffer, base);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window *window = SDL_CreateWindow(
        "Backend start", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Включить Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Включить Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Включить Docking

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);

        {
            update |= RT;
            if (update)
            {
                if (always_random)
                {
                    generate_bits(bits, L);
                    qpsk_mapper_3gpp(bits, symbols);
                    ofdm(symbols, buffer, base);
                }
                if (random_paths)
                channel.set_paths(4, sample_rate);

                channel.pass_through(buffer, signal);
                demodulate_ofdm(signal, demoded, base, start);
                ofdm_equalize(demoded, equalized, base);
                equalized.resize(symbols.size(), {0.0f, 0.0f});
                qpsk_demapper_3gpp(equalized, received_bits);

                compute_spectrum(buffer, buffer_spectrum, buffer_freqs, sample_rate);
                compute_spectrum(signal, signal_spectrum, signal_freqs, sample_rate);

                update = false;
            }

            { // Controls
                static std::mt19937 gen(std::random_device{}());
                static std::uniform_real_distribution<float> dis(0.5f, 1.5f);
                ImGui::Begin("Controls");
                ImGui::Text("FPS: %.2f", io.Framerate);
                ImGui::SliderInt("Packet start", &start, 0, 180);
                if (ImGui::InputInt("Bits count", &L))
                {
                    if (L < 5)
                        L = 66;
                    generate_bits(bits, L);
                    qpsk_mapper_3gpp(bits, symbols);
                    ofdm(symbols, buffer, base);
                }
                if (ImGui::InputFloat("Sample rate", &sample_rate, 1e3f, 1e7f, "%.3e"))
                {
                    channel.set_paths(4, sample_rate);
                    channel.update_paths(dis(gen));
                };
                if (ImGui::InputFloat("Noise dB", &noise_db))
                {
                    channel.set_noise(noise_db);
                };
                if (ImGui::Checkbox("Real-Time", &RT))
                    update = RT;
                if (!RT)
                {
                    ImGui::SameLine();
                    if (ImGui::Button("Update"))
                        update = true;
                }
                ImGui::Checkbox("Random Data", &always_random);
                ImGui::Checkbox("Random Pahts", &random_paths);
                if (ImGui::SliderInt("Pilot Spacing", &PS, 2, N / 2))
                {
                    if (PS > 2)
                        base.pilots_spacing = PS;
                }

                ImGui::End();
            }

            ImGui::Begin("Bits");
            if (ImPlot::BeginPlot("Bits", ImGui::GetContentRegionAvail()))
            {
                ImPlot::PlotStairs("Bits", bits.data(), bits.size());
                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Symbols");
            if (ImPlot::BeginPlot("Symbols##IQ", ImGui::GetContentRegionAvail()))
            {
                ImPlot::PlotLine("I", reinterpret_cast<const float *>(symbols.data()),
                                 symbols.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));
                ImPlot::PlotLine("Q", reinterpret_cast<const float *>(symbols.data()) + 1,
                                 symbols.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));

                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Symbols Constellation");
            if (ImPlot::BeginPlot("SymbolsConstellation##IQ", ImGui::GetContentRegionAvail()))
            {
                float max = 1.3f;
                ImPlot::SetupAxesLimits(-max, max, -max, max, ImPlotCond_Once);
                ImPlot::PlotScatter(
                    "Const",
                    reinterpret_cast<const float *>(symbols.data()),
                    reinterpret_cast<const float *>(symbols.data()) + 1,
                    symbols.size(), 0, 0,
                    sizeof(std::complex<float>));
                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Equalized Constellation");

            if (ImPlot::BeginPlot("Equalized##IQ", ImGui::GetContentRegionAvail()))
            {
                float max = 1.3f;
                ImPlot::SetupAxesLimits(-max, max, -max, max, ImPlotCond_Once);
                ImPlot::PlotScatter(
                    "Const",
                    reinterpret_cast<const float *>(equalized.data()),
                    reinterpret_cast<const float *>(equalized.data()) + 1,
                    equalized.size(), 0, 0,
                    sizeof(std::complex<float>));
                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Buffer");
            if (ImPlot::BeginPlot("Buffer##IQ", ImGui::GetContentRegionAvail()))
            {
                ImPlot::PlotLine("I", reinterpret_cast<const float *>(buffer.data()),
                                 buffer.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));
                ImPlot::PlotLine("Q", reinterpret_cast<const float *>(buffer.data()) + 1,
                                 buffer.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));

                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Signal");
            if (ImPlot::BeginPlot("Signal##IQ", ImGui::GetContentRegionAvail()))
            {
                ImPlot::PlotLine("I", reinterpret_cast<const float *>(signal.data()),
                                 signal.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));
                ImPlot::PlotLine("Q", reinterpret_cast<const float *>(signal.data()) + 1,
                                 signal.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));

                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Equalized");

            if (ImPlot::BeginPlot("Equalized##IQ", ImGui::GetContentRegionAvail()))
            {
                ImPlot::PlotLine("I", reinterpret_cast<const float *>(equalized.data()),
                                 equalized.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));
                ImPlot::PlotLine("Q", reinterpret_cast<const float *>(equalized.data()) + 1,
                                 equalized.size(), 1.0, 0, 0, 0, sizeof(std::complex<float>));

                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Signal Spectrum");
            if (ImPlot::BeginPlot("Signal##Spectrum", ImGui::GetContentRegionAvail()))
            {
                ImPlot::PlotLine("Spectrum", signal_freqs.data(), signal_spectrum.data(), signal_spectrum.size());
                ImPlot::EndPlot();
            }
            ImGui::End();

            ImGui::Begin("Buffer Spectrum");
            if (ImPlot::BeginPlot("Buffer##Spectrum", ImGui::GetContentRegionAvail()))
            {
                ImPlot::PlotLine("Spectrum", buffer_freqs.data(), buffer_spectrum.data(), buffer_spectrum.size());
                ImPlot::EndPlot();
            }
            ImGui::End();
        }

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[])
{

    std::thread gui_thread(run_gui);
    gui_thread.join();

    // Здесь должен работать поток с сервером
    // std::thread zmq_thread(zmq_server_run);
    // zmq_thread.join();

    return 0;
}