﻿using AudioVisualization.Playback;
using Windows.Media.Playback;
using Windows.Foundation.Collections;
using System.Diagnostics;
using AudioVisualization.Controls.Visualizers;

namespace AudioVisualization.Services
{
    class PlayerService
    {
        private static PlayerService _current;

        private PropertySet _sampleGrabberProperties;

        private PropertySet _referenceProperties;


        public static PlayerService Current { get { if (_current == null) { _current = new PlayerService(); } return _current; } }

        private MediaPlayer _mediaPlayer;

        public Playlist Playlist { get; }

        public PropertySet ReferencePropertySet { get { return _referenceProperties; } }

        private PlayerService()
        {
            Playlist = new Playlist();

            InitializeMediaPlayer();
        }

        private void InitializeMediaPlayer()
        {
            _mediaPlayer = new MediaPlayer();

            _mediaPlayer.MediaFailed += _mediaPlayer_MediaFailed;

            _mediaPlayer.MediaEnded += _mediaPlayer_MediaEnded;

            _mediaPlayer.CurrentStateChanged += _mediaPlayer_CurrentStateChanged;

            _mediaPlayer.Source = Playlist.PlaybackList;

            Playlist.PlaybackList.ItemFailed += PlaybackList_ItemFailed;
        }

        private void PlaybackList_ItemFailed(MediaPlaybackList sender, MediaPlaybackItemFailedEventArgs args)
        {
            Debug.WriteLine(args.Error.ToString());
        }

        private void _mediaPlayer_CurrentStateChanged(MediaPlayer sender, object args)
        {
            Debug.WriteLine(sender.CurrentState.ToString());

            // filter property set is ready after opening on the next paused

            if (sender.CurrentState == MediaPlayerState.Playing)
            {
                if (_referenceProperties.ContainsKey("samplegrabber"))
                {
                    SampleGrabber.IMyInterface call = (SampleGrabber.IMyInterface)_referenceProperties["samplegrabber"];

                    var customStruct = call?.GetSingleData();

                    Debug.WriteLine(customStruct.Value);
                    //TODO get a ringbuffer interface
                }
            }
        }

        private void _mediaPlayer_MediaEnded(MediaPlayer sender, object args)
        {
            Debug.WriteLine("Ended");
        }

        private void _mediaPlayer_MediaFailed(MediaPlayer sender, MediaPlayerFailedEventArgs args)
        {
            Debug.WriteLine(args.ErrorMessage);
        }

        public void Play()
        {
            _mediaPlayer.Play();
        }

        public void Pause()
        {
            _mediaPlayer.Pause();
        }

        public void StartVisualization(BaseVisualizer visualizer)
        {
            EnsureSampleGrabber(_mediaPlayer);

            visualizer.SetAudioDataSource(_sampleGrabberProperties);
        }
        private void EnsureSampleGrabber(MediaPlayer player)
        {
            if (_sampleGrabberProperties == null)
            {
                _referenceProperties = new PropertySet();

                player.AddAudioEffect("SampleGrabber.SampleGrabberTransform", false, _referenceProperties);

                if (player.CurrentState == MediaPlayerState.Playing)
                {
                    var oldIndex = Playlist.PlaybackList.CurrentItemIndex;
                    var oldPosition = _mediaPlayer.Position;

                    _mediaPlayer.Source = null;
                    _mediaPlayer.Source = Playlist.PlaybackList;


                    _mediaPlayer.Play();
                    _mediaPlayer.Position = oldPosition;
                }

            }
        }
    }
}
