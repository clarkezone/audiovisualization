using AudioVisualization.Extensions;
using AudioVisualization.Playback;
using AudioVisualization.Services;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace AudioVisualization.Views
{
    /// <summary>
    /// A page that is meant to be used with the AppShell template.
    /// </summary>
    public sealed partial class MainPage : BasePage
    {
        public MainPage()
        {
            this.InitializeComponent();
            CreateStaticResources();
            this.Loaded += MainPage_Loaded;
        }

        private async void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            StorageFile toneText = await StorageFile.GetFileFromApplicationUriAsync(new Uri(@"ms-appx:///Assets/LevelTest_mixdown.mp3"));
            PlayerService.Current.Playlist.Clear();
            PlayerService.Current.Playlist.Add(await toneText.ToSong());

            PlayerService.Current.Playlist.PlaybackList.MoveTo(0);
            PlayerService.Current.Play();
        }

        void CreateStaticResources()
        {
            Services.PlayerService.Current.StartVisualization(win2dVisualizer);
        }
    }
}
