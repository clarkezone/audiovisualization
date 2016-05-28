﻿using AudioVisualization.Controls;
using AudioVisualization.Navigation;
using AudioVisualization.ViewModels;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.System;
using Windows.UI;
using Windows.UI.Core;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace AudioVisualization
{
    /// <summary>
    /// The "chrome" layer of the app that provides top-level navigation with
    /// proper keyboarding navigation.
    /// </summary>
    public sealed partial class AppShell : Page
    {
        public static AppShell Current;
        private Type _lastSourcePageType;
        private readonly AppShellViewModel _viewModel;

        /// <summary>
        /// Initializes a new instance of the AppShell.
        /// </summary>
        public AppShell()
        {
            InitializeComponent();

            // Set the data context
            _viewModel = new AppShellViewModel();

            DataContext = _viewModel;

            Loaded += (sender, args) =>
            {
                Current = this;

                togglePaneButton.Focus(FocusState.Programmatic);

                // We need to update the initial selection because
                // OnNavigatingToPage happens before Items are loaded in
                // both navigation bars.
                UpdateSelectionState(_lastSourcePageType);

                CheckTogglePaneButtonSizeChanged();
            };

            rootSplitView.RegisterPropertyChangedCallback(
                SplitView.DisplayModeProperty,
                (s, a) =>
                {
                    // Ensure that we update the reported size of the TogglePaneButton when the SplitView's
                    // DisplayMode changes.
                    CheckTogglePaneButtonSizeChanged();
                });

            SystemNavigationManager.GetForCurrentView().BackRequested += SystemNavigationManager_BackRequested;

            var titleBar = ApplicationView.GetForCurrentView().TitleBar;
            titleBar.ButtonBackgroundColor = Colors.White;
            titleBar.ButtonHoverBackgroundColor = (Color)Application.Current.Resources["AppAccentLightColor"];
            titleBar.ButtonPressedBackgroundColor = (Color)Application.Current.Resources["AppAccentColor"];
            titleBar.ButtonForegroundColor = (Color)Application.Current.Resources["AppAccentForegroundColor"];
            titleBar.ButtonHoverForegroundColor = (Color)Application.Current.Resources["AppAccentForegroundColor"];
        }

        /// <summary>
        /// Gets the app frame.
        /// </summary>
        public Frame AppFrame
        {
            get { return frame; }
        }

        /// <summary>
        /// The rectangle of the toggle button.
        /// </summary>
        public Rect TogglePaneButtonRect { get; private set; }

        /// <summary>
        /// Default keyboard focus movement for any unhandled keyboarding
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The event arguments</param>
        private void AppShell_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            var direction = FocusNavigationDirection.None;
            switch (e.Key)
            {
                case VirtualKey.Left:
                case VirtualKey.GamepadDPadLeft:
                case VirtualKey.GamepadLeftThumbstickLeft:
                case VirtualKey.NavigationLeft:
                    direction = FocusNavigationDirection.Left;
                    break;
                case VirtualKey.Right:
                case VirtualKey.GamepadDPadRight:
                case VirtualKey.GamepadLeftThumbstickRight:
                case VirtualKey.NavigationRight:
                    direction = FocusNavigationDirection.Right;
                    break;

                case VirtualKey.Up:
                case VirtualKey.GamepadDPadUp:
                case VirtualKey.GamepadLeftThumbstickUp:
                case VirtualKey.NavigationUp:
                    direction = FocusNavigationDirection.Up;
                    break;

                case VirtualKey.Down:
                case VirtualKey.GamepadDPadDown:
                case VirtualKey.GamepadLeftThumbstickDown:
                case VirtualKey.NavigationDown:
                    direction = FocusNavigationDirection.Down;
                    break;
            }

            if (direction != FocusNavigationDirection.None)
            {
                var control = FocusManager.FindNextFocusableElement(direction) as Control;
                if (control != null)
                {
                    control.Focus(FocusState.Programmatic);
                    e.Handled = true;
                }
            }
        }

        /// <summary>
        /// Check for the conditions where the navigation pane does not occupy the space under the floating
        /// hamburger button and trigger the event.
        /// </summary>
        private void CheckTogglePaneButtonSizeChanged()
        {
            if (rootSplitView.DisplayMode == SplitViewDisplayMode.Inline ||
                rootSplitView.DisplayMode == SplitViewDisplayMode.Overlay)
            {
                var transform = togglePaneButton.TransformToVisual(this);
                var rect =
                    transform.TransformBounds(new Rect(0, 0, togglePaneButton.ActualWidth,
                        togglePaneButton.ActualHeight));
                TogglePaneButtonRect = rect;
            }
            else
            {
                TogglePaneButtonRect = new Rect();
            }

            TogglePaneButtonRectChanged?.DynamicInvoke(this, TogglePaneButtonRect);
        }

        /// <summary>
        /// Enable accessibility on each nav menu item by setting the AutomationProperties.Name on each container
        /// using the associated Label of each item.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="args">The event arguments.</param>
        private void NavMenuItemContainerContentChanging(ListViewBase sender, ContainerContentChangingEventArgs args)
        {
            if (!args.InRecycleQueue && args.Item is INavigationBarMenuItem)
            {
                args.ItemContainer.SetValue(AutomationProperties.NameProperty,
                    ((INavigationBarMenuItem)args.Item).Label);
            }
            else
            {
                args.ItemContainer.ClearValue(AutomationProperties.NameProperty);
            }
        }

        /// <summary>
        /// Navigate to the Page for the selected <paramref name="listViewItem" />.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="listViewItem">The ListViewItem.</param>
        private void NavMenuList_ItemInvoked(object sender, ListViewItem listViewItem)
        {
            var item = (INavigationBarMenuItem)((NavMenuListView)sender).ItemFromContainer(listViewItem);

            if (item?.DestPage != null &&
                item.DestPage != AppFrame.CurrentSourcePageType)
            {
                AppFrame.Navigate(item.DestPage, item.Arguments);
            }
        }

        private void OnNavigatedToPage(object sender, NavigationEventArgs e)
        {
            // After a successful navigation set keyboard focus to the loaded page
            if (e.Content is Page)
            {
                var control = (Page)e.Content;
                control.Loaded += Page_Loaded;
            }

            // Update the Back button depending on whether we can go Back.
            SystemNavigationManager.GetForCurrentView().AppViewBackButtonVisibility =
                AppFrame.CanGoBack ?
                AppViewBackButtonVisibility.Visible :
                AppViewBackButtonVisibility.Collapsed;

            _lastSourcePageType = e.SourcePageType;

            UpdateSelectionState(e.SourcePageType);
        }

        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            ((Page)sender).Focus(FocusState.Programmatic);
            ((Page)sender).Loaded -= Page_Loaded;
        }

        private void SystemNavigationManager_BackRequested(object sender, BackRequestedEventArgs e)
        {
            if (!e.Handled && AppFrame.CanGoBack)
            {
                e.Handled = true;
                AppFrame.GoBack();
            }
        }

        /// <summary>
        /// Callback when the SplitView's Pane is toggled open or close.  When the Pane is not visible
        /// then the floating hamburger may be occluding other content in the app unless it is aware.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The event arguments.</param>
        private void TogglePaneButton_Checked(object sender, RoutedEventArgs e)
        {
            CheckTogglePaneButtonSizeChanged();
        }

        /// <summary>
        /// An event to notify listeners when the hamburger button may occlude other content in the app.
        /// The custom "PageHeader" user control is using this.
        /// </summary>
        public event TypedEventHandler<AppShell, Rect> TogglePaneButtonRectChanged;

        private void UpdateSelectionState(Type sourcePageType)
        {
            navMenuList.SetSelectedItem(null);
            bottomNavMenuList.SetSelectedItem(null);

            INavigationBarMenuItem item = (from p in _viewModel.NavigationBarMenuItems.Union(_viewModel.BottomNavigationBarMenuItems)
                                           where p.DestPage == sourcePageType
                                           select p)
                                          .SingleOrDefault();

            togglePaneButton.Background = item == null ?
                new SolidColorBrush((Color)Application.Current.Resources["AppAccentLightColor"]) :
                new SolidColorBrush(Colors.Transparent);

            ListViewItem container = null;

            if (navMenuList.Items != null && navMenuList.Items.Contains(item))
            {
                container = (ListViewItem)navMenuList.ContainerFromItem(item);
            }
            else if (bottomNavMenuList.Items != null && bottomNavMenuList.Items.Contains(item))
            {
                container = (ListViewItem)bottomNavMenuList.ContainerFromItem(item);
            }

            // While updating the selection state of the item prevent it from taking keyboard focus. If a
            // user is invoking the back button via the keyboard causing the selected nav menu item to change
            // then focus will remain on the back button.
            if (container != null)
            {
                container.IsTabStop = false;
            }

            navMenuList.SetSelectedItem(container);
            bottomNavMenuList.SetSelectedItem(container);

            if (container != null)
            {
                container.IsTabStop = true;
            }
        }
    }
}
